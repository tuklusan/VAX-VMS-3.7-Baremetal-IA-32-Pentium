#ifndef PORT_INI_BOOTDEV_SCAN_H
#define PORT_INI_BOOTDEV_SCAN_H
#include <stdint.h>
#include "vms_iniloa_iomap.h"
/* Original SYS/SRC/INIT.MAR INI_BOOTDEVIC, adapter-list walk near
 * 1311-1330.  CPU-dependent adapter registration is provided by the
 * real IA-32 INILOA I/O map, not a fabricated VAX UNIBUS.  This stage
 * selects a verified boot adapter; it does NOT allocate a DDB or UCB. */
enum { PORT_INI_BOOTDEV_OK=1u, PORT_INI_BOOTDEV_BAD_ARGUMENT=2u,
       PORT_INI_BOOTDEV_AMBIGUOUS=4u, PORT_INI_BOOTDEV_NO_MATCH=6u };
struct port_ini_bootdev_result {
    uint32_t boot_index;
    uint32_t boot_kind;
    uint32_t boot_io_base;
    uint32_t boot_kind_unit;
    uint32_t scanned_adapters;
};
uint32_t port_ini_bootdev_scan(const struct port_ini_io_state *mapped,
                              struct port_ini_bootdev_result *result);
#endif
