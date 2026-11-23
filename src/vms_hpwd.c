#include "vms_hpwd.h"

/* The 32-bit CRC instruction uses this reversed AUTODIN-II polynomial. */
static uint32_t hpwd_crc32(const uint8_t *password, uint16_t len) {
    uint32_t value = UINT32_MAX;
    for (uint32_t i = 0; i < len; ++i) {
        value ^= password[i];
        for (unsigned bit = 0; bit < 8; ++bit)
            value = (value >> 1) ^ ((value & 1u) ? 0xedb88320u : 0u);
    }
    return value;
}

#define HPWD_P (UINT64_MAX - UINT64_C(58))

/* Arithmetic modulo 2^64-59 without a 64-bit divide or 128-bit runtime. */
static uint64_t hpwd_add(uint64_t a, uint64_t b) {
    uint64_t sum = a + b;
    if (sum < a)
        return sum + UINT64_C(59);
    return sum >= HPWD_P ? sum - HPWD_P : sum;
}

static uint64_t hpwd_mul(uint64_t a, uint64_t b) {
    uint64_t product = 0;
    while (b) {
        if (b & 1u)
            product = hpwd_add(product, a);
        b >>= 1;
        if (b)
            a = hpwd_add(a, a);
    }
    return product;
}

static uint64_t hpwd_power(uint64_t base, uint32_t exponent) {
    uint64_t value = 1;
    while (exponent) {
        if (exponent & 1u)
            value = hpwd_mul(value, base);
        exponent >>= 1;
        if (exponent)
            base = hpwd_mul(base, base);
    }
    return value;
}

/* VAX COLLAPSE counts backward from the descriptor length. The source
 * bytes advance forward, with one-byte carry discarded at each addition. */
static void hpwd_collapse(uint8_t bytes[8], const uint8_t *input, uint16_t len) {
    for (uint32_t i = 0; i < len; ++i) {
        unsigned byte = (len - i) & 7u;
        bytes[byte] = (uint8_t)(bytes[byte] + input[i]);
    }
}

static uint64_t hpwd_purdy(uint64_t x) {
    if (x >= HPWD_P)
        x -= HPWD_P;

    /* The HPWD polynomial, evaluated in its original six-term order. */
    uint64_t high = hpwd_add(hpwd_power(x, 16777213u),
                             hpwd_mul(HPWD_P - 83u, hpwd_power(x, 16777153u)));
    uint64_t low = hpwd_mul(HPWD_P - 179u, x);
    low = hpwd_mul(hpwd_add(low, HPWD_P - 257u), x);
    low = hpwd_mul(hpwd_add(low, HPWD_P - 323u), x);
    low = hpwd_add(low, HPWD_P - 363u);
    return hpwd_add(high, low);
}

int port_hpwd(uint8_t hash[8], const uint8_t *password, uint16_t password_len,
              uint8_t algorithm, uint16_t salt,
              const uint8_t *username, uint16_t username_len) {
    if (!hash || (password_len && !password) ||
        (algorithm != 0u && username_len && !username))
        return 0;

    if (algorithm == PORT_HPWD_CRC) {
        uint32_t crc = hpwd_crc32(password, password_len);
        for (unsigned i = 0; i < 4; ++i)
            hash[i] = (uint8_t)(crc >> (8u * i));
        for (unsigned i = 4; i < 8; ++i)
            hash[i] = 0;
        return 1;
    }

    uint8_t bytes[8] = {0};
    hpwd_collapse(bytes, password, password_len);
    uint16_t middle = (uint16_t)((uint16_t)bytes[3] |
                                  ((uint16_t)bytes[4] << 8));
    middle = (uint16_t)(middle + salt);
    bytes[3] = (uint8_t)middle;
    bytes[4] = (uint8_t)(middle >> 8);
    hpwd_collapse(bytes, username, username_len);
    uint64_t x = 0;
    for (unsigned i = 0; i < 8; ++i)
        x |= (uint64_t)bytes[i] << (8u * i);
    uint64_t result = hpwd_purdy(x);
    for (unsigned i = 0; i < 8; ++i)
        hash[i] = (uint8_t)(result >> (8u * i));
    return 1;
}

int port_hpwd_matches(const uint8_t stored[8], const uint8_t *password,
                      uint16_t password_len, uint8_t algorithm, uint16_t salt,
                      const uint8_t *username, uint16_t username_len) {
    uint8_t computed[8];
    if (!stored || !port_hpwd(computed, password, password_len,
                              algorithm, salt, username, username_len))
        return 0;
    unsigned mismatch = 0;
    for (unsigned i = 0; i < 8; ++i) {
        mismatch |= (unsigned)(stored[i] ^ computed[i]);
        computed[i] = 0;
    }
    return mismatch == 0;
}
