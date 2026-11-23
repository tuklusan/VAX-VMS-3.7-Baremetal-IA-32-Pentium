#ifndef VMS_INITVEC_H
#define VMS_INITVEC_H

#include <stddef.h>
#include <stdint.h>

/* SYS/SRC/INITVEC.MAR: one INILOA entry and its self-relative image header. */
enum {
    PORT_INI_LOAD_HEADER_BYTES = 12u,
    PORT_INI_IMAGE_ENTRY_BYTES = 5u,
    PORT_INI_IMAGE_TERMINATOR_BYTES = 4u,
    PORT_INI_TYPE_IOMAP = 3u,
    PORT_INI_DYN_LOADCODE = 98u,
    PORT_INI_OK = 1u,
    PORT_INI_NO_IMAGE = 0u,
    PORT_INI_BAD_IMAGE = 2u,
    PORT_INI_BAD_ENTRY = 4u
};

/* A loaded entry is only a pointer to original startup code. It is not a
 * completed adapter initialization; the image still has to be loaded. */
struct port_ini_vectors {
    uintptr_t iomap;
    uint32_t image_size;
    int linked;
};

void port_ini_vectors_init(struct port_ini_vectors *vectors);

/* Resolve the source-defined type-3 entry in a self-describing INILOA image.
 * Data and code remain inside the caller-owned executable image region. */
uint32_t port_ini_vectors_link(struct port_ini_vectors *vectors,
                               const void *image, size_t loaded_bytes);

/* The source calls INI$IOMAP after linking and before freeing INILOA pool. */
uint32_t port_ini_iomap_call(const struct port_ini_vectors *vectors);

#endif
