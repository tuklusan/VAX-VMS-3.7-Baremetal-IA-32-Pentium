#include "vms_initvec.h"

static uint16_t read_u16(const uint8_t *b) {
    return (uint16_t)((uint16_t)b[0] | ((uint16_t)b[1] << 8));
}
static uint32_t read_u32(const uint8_t *b) {
    return (uint32_t)b[0] | ((uint32_t)b[1] << 8) |
           ((uint32_t)b[2] << 16) | ((uint32_t)b[3] << 24);
}
void port_ini_vectors_init(struct port_ini_vectors *vectors) {
    if (!vectors) return;
    vectors->iomap = 0;
    vectors->image_size = 0;
    vectors->linked = 0;
}
uint32_t port_ini_vectors_link(struct port_ini_vectors *vectors,
                               const void *image, size_t loaded_bytes) {
    if (!vectors) return PORT_INI_BAD_IMAGE;
    port_ini_vectors_init(vectors);
    if (!image || !loaded_bytes) return PORT_INI_NO_IMAGE;
    if (loaded_bytes < PORT_INI_LOAD_HEADER_BYTES +
                       PORT_INI_IMAGE_ENTRY_BYTES +
                       PORT_INI_IMAGE_TERMINATOR_BYTES)
        return PORT_INI_BAD_IMAGE;
    const uint8_t *b = image;
    /* The first longword records the size of the loadable code, and the
     * second half of the header repeats it as a 16-bit size. */
    uint32_t declared = read_u32(b);
    uint32_t second = (uint32_t)read_u16(b + 8u);
    if (read_u32(b + 4u) != 0u || b[10u] != PORT_INI_DYN_LOADCODE || b[11u] != 0u ||
        declared < PORT_INI_LOAD_HEADER_BYTES + 9u ||
        declared > loaded_bytes || declared > 65535u || second != declared)
        return PORT_INI_BAD_IMAGE;

    /* INITVEC defines one type-3 unaligned JMP, followed by -1.
     * R2 is at the displacement word after the type byte. */
    if (b[12u] != PORT_INI_TYPE_IOMAP ||
        read_u32(b + 17u) != 0xFFFFFFFFu)
        return PORT_INI_BAD_ENTRY;
    uint32_t offset = read_u32(b + 13u);
    uint32_t target = 13u + offset; /* Original address arithmetic is 32 bit. */
    if (target < 21u || target >= declared)
        return PORT_INI_BAD_ENTRY;
    vectors->iomap = (uintptr_t)(b + target);
    vectors->image_size = declared;
    vectors->linked = 1;
    return PORT_INI_OK;
}
uint32_t port_ini_iomap_call(const struct port_ini_vectors *vectors) {
    if (!vectors || !vectors->linked || !vectors->iomap)
        return PORT_INI_NO_IMAGE;
    uint32_t (*entry)(void) = (uint32_t (*)(void))vectors->iomap;
    return entry();
}
