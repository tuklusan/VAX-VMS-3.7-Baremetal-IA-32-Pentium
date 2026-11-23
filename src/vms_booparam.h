#ifndef VMS_BOOPARAM_H
#define VMS_BOOPARAM_H

#include <stdint.h>
#include <stddef.h>

/* SYS/SRC/BOOPARAM.MAR: the native SYSBOOT-to-INIT parameter block.
 * Original VAX pointers have 32-bit storage in both implementations. */
struct port_boo_parameters {
    uint32_t disk_driver;
    uint32_t sysloa;
    uint32_t terminal_driver;
    uint32_t iniloa_address;
    uint32_t iniloa_bytes;
    uint32_t nonpaged_dynamic_bytes;
    uint32_t irp_split_address;
    uint32_t irp_count;
    uint32_t lrp_size;
    uint32_t lrp_minimum;
    uint32_t lrp_split_address;
    uint32_t lrp_count;
    uint32_t srp_split_address;
    uint32_t srp_count;
    uint32_t file_cache_bytes;
    uint32_t file_cache_address;
    uint32_t boot_control;
    uint8_t top_system[10];
    uint8_t scs_system_id[6];
    uint32_t port_driver;
    uint32_t port_microcode;
    uint32_t scs_load;
};

_Static_assert(sizeof(struct port_boo_parameters)==96u,"SYSBOOT parameter size");
_Static_assert(offsetof(struct port_boo_parameters,irp_count)==28u,"IRP offset");
_Static_assert(offsetof(struct port_boo_parameters,lrp_count)==44u,"LRP offset");
_Static_assert(offsetof(struct port_boo_parameters,srp_count)==52u,"SRP offset");
_Static_assert(offsetof(struct port_boo_parameters,boot_control)==64u,"BOOTCB offset");
_Static_assert(offsetof(struct port_boo_parameters,top_system)==68u,"TOPSYS offset");
_Static_assert(offsetof(struct port_boo_parameters,port_driver)==84u,"PORT offset");

void port_boo_parameters_init(struct port_boo_parameters *p);
void port_boo_parameters_handoff(struct port_boo_parameters *to,
                                  const struct port_boo_parameters *from);

#endif
