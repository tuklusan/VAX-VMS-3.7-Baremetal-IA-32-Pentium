#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include "vms_initvec.h"

static void store16(uint8_t *b, unsigned v) {
    b[0] = (uint8_t)v;
    b[1] = (uint8_t)(v >> 8);
}
static void store32(uint8_t *b, uint32_t v) {
    for (unsigned i = 0; i < 4; ++i) b[i] = (uint8_t)(v >> (8u * i));
}
int main(void) {
    struct port_ini_vectors v;
    uint8_t image[64] = {0};
    port_ini_vectors_init(&v);
    assert(v.iomap == 0 && !v.linked);
    assert(port_ini_vectors_link(&v, NULL, 0) == PORT_INI_NO_IMAGE);
    assert(port_ini_vectors_link(&v, image, 18) == PORT_INI_BAD_IMAGE);
    assert(port_ini_iomap_call(&v) == PORT_INI_NO_IMAGE);
    store32(image, sizeof image);
    store16(image + 8, sizeof image);
    image[10] = PORT_INI_DYN_LOADCODE;
    image[12] = PORT_INI_TYPE_IOMAP;
    store32(image + 13, 32 - 13);
    store32(image + 17, 0xFFFFFFFFu);
    assert(port_ini_vectors_link(&v, image, sizeof image) == PORT_INI_OK);
    assert(v.linked && v.iomap == (uintptr_t)(image + 32));
    assert(v.image_size == sizeof image);
    image[12] = 1;
    assert(port_ini_vectors_link(&v, image, sizeof image) == PORT_INI_BAD_ENTRY);
    assert(!v.linked);
    image[12] = PORT_INI_TYPE_IOMAP;
    store32(image + 13, (uint32_t)(0 - 13));
    assert(port_ini_vectors_link(&v, image, sizeof image) == PORT_INI_BAD_ENTRY);
    store32(image + 13, 32 - 13);
    store32(image + 17, 0);
    assert(port_ini_vectors_link(&v, image, sizeof image) == PORT_INI_BAD_ENTRY);
    store32(image + 17, 0xFFFFFFFFu);
    image[10] = 0;
    assert(port_ini_vectors_link(&v, image, sizeof image) == PORT_INI_BAD_IMAGE);
    image[10] = PORT_INI_DYN_LOADCODE;
    store16(image + 8, 31);
    assert(port_ini_vectors_link(&v, image, sizeof image) == PORT_INI_BAD_IMAGE);
    store16(image + 8, sizeof image);
    assert(port_ini_vectors_link(&v, image, sizeof image) == PORT_INI_OK);
    puts("PASS: source INILOA header, self-relative INITVEC, absent image, invalid entries");
    return 0;
}
