#ifndef VMS_VMB_RPB_H
#define VMS_VMB_RPB_H
#include <stdint.h>
#include "vms_vmbargs.h"

/* BOOTS/SRC/VMB.MAR's RPB construction and PFN descriptor handoff.
 * The IA-32 E820 ranges replace VAX memory-controller descriptions. */
enum {PORT_VMB_RPB_MAX_RANGES=32u};
enum {PORT_VMB_RPB_BAD_ARGUMENT=2u,PORT_VMB_RPB_BAD_MAP=4u,
      PORT_VMB_RPB_NO_DRIVER=6u,PORT_VMB_RPB_MANY_RANGES=8u};
struct port_vmb_memory_descriptor {
    uint32_t start_pfn;
    uint32_t pages;
};
/* VMB.MAR saves incoming R0-R5 in RPB$L_BOOTR0...BOOTR5.  The PC
 * counterparts hold native device I/O ports and boot flags; the two
 * VAX-only register meanings have no physical PC value. */
struct port_vmb_boot_registers {
    uint32_t device_type;
    uint32_t adapter_port;
    uint32_t controller_port;
    uint32_t unit;
    uint32_t adapter_flags;
    uint32_t software_flags;
};
struct port_vmb_restart_block {
    uint32_t base_address;
    uint32_t boot_registers[6];
    uint32_t halt_pc, halt_psl, halt_code;
    uint32_t pfn_bitmap_bytes, pfn_bitmap_address;
    uint32_t free_page_count;
    uint32_t pfn_low, pfn_high;
    uint16_t boot_unit;
    uint8_t boot_device_type, reserved;
    const struct boo_io_driver *boot_driver;
    uint32_t memory_descriptor_count;
    struct port_vmb_memory_descriptor memory[PORT_VMB_RPB_MAX_RANGES];
};
/* The supplied boot-driver vector must have been selected, and its map
 * must describe actual E820 memory, with no missing/overlapping pages. */
uint32_t port_vmb_init_restart(struct port_vmb_restart_block *rpb,
                          const struct port_vmb_arguments *args,
                          const uint8_t *bitmap,uint32_t bitmap_capacity,
                          const struct port_pc_memory_range *e820,
                          uint32_t e820_count,
                          const struct boo_io_driver *driver,
                          const struct port_vmb_boot_registers *boot);
#endif
