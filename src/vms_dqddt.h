#ifndef VMS37_NATIVE_DQDDT_H
#define VMS37_NATIVE_DQDDT_H
#include <stdint.h>
#include <stddef.h>
#include "vms_dq_fdt.h"
#include "vms_dq_bootread.h"
/* Original DRIVER/SRC/DQDRIVER.MAR DPTAB 392-436, DDTAB 438-465,
 * FUNCTAB 496-592 and SHRLIB/SYSMAR.MAR DDTAB/GENRADDR macro.
 * Only the source-layout table *stage* is usable: the PC ATA READPBLK
 * adapter does not implement VAX DQ_STARTIO register ABI; all DDT actions
 * (other than exact masks) remain unbound. Never expose as IOC live driver. */
enum {PORT_DQDDT_OK=1u,PORT_DQDDT_BAD=2u,PORT_DQDDT_UNBOUND=4u,
      PORT_DQDDT_HEADER_BYTES=40u,PORT_DQDDT_DDT_BYTES=38u,
      PORT_DQDDT_FDT_BYTES=136u,PORT_DQDDT_IMAGE_BYTES=176u,
      PORT_DQDDT_SOURCE_DIAG_BYTES=84u,
      PORT_DQDDT_ACTIONS=10u};
struct port_dqddt_prologue {
    uint16_t max_units,default_units,buffer_bytes;
    uint8_t fork_ipl,device_ipl,retry_max,svp_required;
};
struct port_dqddt_stage {
    struct port_dqddt_prologue dpt;
    uint8_t image[PORT_DQDDT_IMAGE_BYTES] __attribute__((aligned(4)));
    uint32_t table_base,native_start_pc,ioc_return_pc;
    uint8_t ready,driver_bound,reserved[2];
};
uint32_t port_dqddt_preflight(const struct port_dqddt_stage *,
    const struct port_dq_fdt *,uint32_t,uint32_t,uint32_t);
uint32_t port_dqddt_fill_header(uint8_t *,size_t,uint32_t,uint32_t,uint32_t);
uint32_t port_dqddt_fill_fdt(uint8_t *,size_t,const struct port_dq_fdt *);
uint32_t port_dqddt_stage_init(struct port_dqddt_stage *,
    const struct port_dq_fdt *,uint32_t,uint32_t,uint32_t);
uint32_t port_dqddt_validate(const struct port_dqddt_stage *,
    const struct port_dq_fdt *);
uint32_t port_dqddt_function(const struct port_dqddt_stage *,
    const struct port_dq_fdt *,uint16_t);
/* Execute only the existing privileged physical READPBLK path through a
 * validated staged DDT. This does not bind the general driver or public QIO. */
uint32_t port_dqddt_bootread(const struct port_dqddt_stage *,
    const struct port_dq_fdt *,const struct port_dq_bootread_request *,
    struct port_dq_bootread_result *,uint32_t);
uint32_t port_dqddt_entry_offset(unsigned);
uint32_t port_dqddt_fdt_check(const struct port_dqddt_stage *,
    const struct port_dq_fdt *);
#endif
