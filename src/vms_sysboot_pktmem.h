#ifndef VMS_SYSBOOT_PKTMEM_H
#define VMS_SYSBOOT_PKTMEM_H
#include <stdint.h>
#include "vms_booparam.h"
/* BOOTS/SRC/SYSBOOT.MAR: NPAGE_SPT, IRP_SPT, LRP_SPT, SRP_SPR and
 * ALLOC_SRP/ALLOC_IRP/ALLOC_LRP. A 512-byte page and a 16-byte packet
 * alignment are part of the original boot ABI. Sizes come from SYSGEN;
 * the native caller, not this routine, supplies the actual mapped pages. */
enum { PORT_BOO_PACKET_OK=1u, PORT_BOO_PACKET_BADPARAM=2u,
       PORT_BOO_PACKET_NOMEM=4u, PORT_BOO_PACKET_PAGE=512u };
struct port_boo_packet_settings {
    uint32_t top_address;
    uint32_t irp_length, irp_count, irp_virtual_count;
    uint32_t lrp_requested_size, lrp_count, lrp_virtual_count;
    uint32_t srp_requested_size, srp_count, srp_virtual_count;
    uint32_t nonpaged_dynamic_bytes, nonpaged_virtual_bytes, paged_dynamic_bytes;
};
struct port_boo_packet_range {
    uint32_t first_address, resident_end, reserved_end;
    uint32_t packet_bytes, resident_count, virtual_count;
};
struct port_boo_packet_layout {
    struct port_boo_packet_range srp, irp, lrp;
    uint32_t nonpaged_start, nonpaged_end, nonpaged_virtual_end;
    uint32_t paged_dynamic_start;
    uint32_t lrp_minimum;
};
uint32_t port_boo_packet_layout(const struct port_boo_packet_settings *settings,
                    struct port_boo_packet_layout *layout,
                    struct port_boo_parameters *booparam);
#endif
