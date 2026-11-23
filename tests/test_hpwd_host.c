#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include "vms_hpwd.h"

struct hpwd_vector {
    const uint8_t *password;
    uint16_t pw_len;
    const uint8_t *username;
    uint16_t user_len;
    uint16_t salt;
    uint8_t algorithm;
    uint8_t result[8];
};
#define LIT_BYTES(s) (const uint8_t *)(s), (uint16_t)(sizeof(s) - 1u)
static const struct hpwd_vector vectors[] = {
    {LIT_BYTES(""), LIT_BYTES(""), 0, 0, {0xff,0xff,0xff,0xff,0,0,0,0}},
    {LIT_BYTES("PASSWORD"), LIT_BYTES("SYSTEM"), 0, 0, {0x66,0x25,0x0e,0x0c,0,0,0,0}},
    {LIT_BYTES("PASSWORD"), LIT_BYTES("SYSTEM"), 0, 1, {0x6f,0x65,0xdd,0xb7,0xcc,0xfb,0x0a,0x17}},
    {LIT_BYTES("PASSWORD"), LIT_BYTES("SYSTEM"), 1234, 1, {0x00,0x4a,0xd8,0x22,0x63,0xbb,0xe2,0xb7}},
    {LIT_BYTES("PASSWORD"), LIT_BYTES("ALICE"), 1234, 1, {0xda,0x0b,0x48,0xa5,0xb7,0x5b,0x14,0x24}},
    {LIT_BYTES(""), LIT_BYTES(""), 0, 1, {0x5a,0xfe,0xff,0xff,0xff,0xff,0xff,0xff}},
    {LIT_BYTES("123456789"), LIT_BYTES("USER"), 65535, 5, {0x40,0x1d,0xf9,0x19,0xc2,0x4b,0x4d,0x57}},
    {LIT_BYTES("ABCDEFGHIJKLMNOPQRSTUVWXYZ"), LIT_BYTES("SYSTEM"), 1, 1, {0xbe,0xd0,0x4b,0x62,0x1f,0x7d,0x3b,0x09}}
};

int main(void) {
    uint8_t hash[8];
    for (unsigned i = 0; i < sizeof vectors / sizeof vectors[0]; ++i) {
        const struct hpwd_vector *v = &vectors[i];
        if (!port_hpwd(hash,v->password,v->pw_len,v->algorithm,v->salt,
                       v->username,v->user_len) || memcmp(hash,v->result,8)) {
            fprintf(stderr,"HPWD hash vector %u failed\n",i);
            return 1;
        }
        if (!port_hpwd_matches(v->result,v->password,v->pw_len,v->algorithm,
                               v->salt,v->username,v->user_len)) return 2;
    }
    if(port_hpwd(NULL,NULL,0,0,0,NULL,0) ||
       port_hpwd(hash,NULL,1,0,0,NULL,0) ||
       port_hpwd(hash,(const uint8_t *)"P",1,1,0,NULL,1) ||
       port_hpwd_matches(vectors[3].result,(const uint8_t *)"PASSWORd",8,1,1234,
                         (const uint8_t *)"SYSTEM",6)) return 3;
    puts("PASS: full LOGIN HPWD hash algorithms and descriptor handling");
    return 0;
}
