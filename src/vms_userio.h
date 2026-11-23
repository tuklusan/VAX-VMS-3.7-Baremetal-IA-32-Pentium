#ifndef PORTLAB_USERIO_H
#define PORTLAB_USERIO_H
#include <stdint.h>
#include "vms_userptr.h"
/* Experimental CPL3 QIO submit descriptor; NOT the SYS$QIO argument ABI. */
struct port_user_io_desc {
    uint32_t lba;
    uint32_t data_va;
    uint32_t iosb_va;
};
_Static_assert(sizeof(struct port_user_io_desc)==12,"QIO descriptor layout");
/* Decode an untrusted descriptor through the caller's user page tables and
 * check both output ranges. All result fields remain unchanged on failure. */
int port_user_io_decode(const struct port_user_map *map,
                        uint32_t descriptor_va,
                        port_phys_page_resolver physical,void *context,
                        struct port_user_io_desc *result);
#endif
