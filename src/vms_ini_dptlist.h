#ifndef VMS_INI_DPTLIST_H
#define VMS_INI_DPTLIST_H
#include <stddef.h>
#include <stdint.h>
/* Original SYS/SRC/INIT.MAR 1232-1239, and DEVICEDAT.MAR 192-196:
 * IOC$GL_DPTLIST is a circular doubly-linked 8-byte queue sentinel.
 * INSQUE MB$DPT, then NL$DPT, then OP$DPT at its head; the resulting
 * forward order is OP -> NL -> MB. These are *addresses of actual linked
 * DPT records*, not replacement driver implementations or fake dispatch.
 *
 * Guest addresses are 32-bit. The flat memory window provides a safe
 * way for host tests to check original VAX INSQUE pointer bytes. Callers
 * must supply real prologue records before integrating into SYS.EXE. */
enum {PORT_INI_DPT_OK=1u,PORT_INI_DPT_BADPARAM=2u,
      PORT_INI_DPT_NOTEMPTY=4u};
struct port_ini_dpt_result {
    uint32_t head_address;
    uint32_t first_driver;
    uint32_t last_driver;
    uint32_t driver_count;
};
uint32_t port_ini_dpt_attach_three(uint8_t *memory,size_t capacity,
    uint32_t guest_memory_base,uint32_t sentinel_address,
    uint32_t mb_driver_address,uint32_t nl_driver_address,
    uint32_t op_driver_address,struct port_ini_dpt_result *out);
#endif
