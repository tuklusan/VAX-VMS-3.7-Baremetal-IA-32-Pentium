#ifndef VMS37_NATIVE_DQDDT_H
#define VMS37_NATIVE_DQDDT_H
#include <stdint.h>
#include <stddef.h>
#include "vms_dq_fdt.h"
#include "vms_dq_bootread.h"
/* Original DRIVER/SRC/DQDRIVER.MAR DPTAB 392-436, DDTAB 438-465,
 * FUNCTAB 496-592 and SHRLIB/SYSMAR.MAR DDTAB/GENRADDR macro.
 * The source-layout table remains private: native READPBLK and WRITEPBLK
 * action slots execute only privileged physical IRPs; other DDT actions,
 * public QIO and the Files-11 ACP remain unimplemented. */
enum {PORT_DQDDT_OK=1u,PORT_DQDDT_BAD=2u,PORT_DQDDT_UNBOUND=4u,
      PORT_DQDDT_NATIVE=8u,
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
    uint32_t native_read_action_pc,native_write_action_pc;
    uint8_t ready,driver_bound,reserved[2];
};
/* Native-pointer form of the source DDT/FDT, published into DQA0 DDB/UCB.
 * Its supported actions remain limited to privileged physical block I/O. */
struct port_dqddt_native {
    uint32_t base;
    uint8_t ready,reserved[3];
    uint8_t image[PORT_DQDDT_IMAGE_BYTES] __attribute__((aligned(4)));
};
uint32_t port_dqddt_preflight(const struct port_dqddt_stage *,
    const struct port_dq_fdt *,uint32_t,uint32_t,uint32_t);
uint32_t port_dqddt_fill_header(uint8_t *,size_t,uint32_t,uint32_t,uint32_t);
uint32_t port_dqddt_fill_fdt(uint8_t *,size_t,const struct port_dq_fdt *,
    uint32_t,uint32_t,uint32_t);
uint32_t port_dqddt_stage_init(struct port_dqddt_stage *,
    const struct port_dq_fdt *,uint32_t,uint32_t,uint32_t,uint32_t,uint32_t);
uint32_t port_dqddt_validate(const struct port_dqddt_stage *,
    const struct port_dq_fdt *);
uint32_t port_dqddt_activate(const struct port_dqddt_stage *,
    const struct port_dq_fdt *,struct port_dqddt_native *,uint32_t);
uint32_t port_dqddt_native_validate(const struct port_dqddt_stage *,
    const struct port_dq_fdt *,const struct port_dqddt_native *);
uint32_t port_dqddt_function(const struct port_dqddt_stage *,
    const struct port_dq_fdt *,uint16_t);
/* Execute only the existing privileged physical READPBLK path through a
 * validated staged DDT. This does not implement public QIO or ACP behavior. */
uint32_t port_dqddt_bootread(const struct port_dqddt_stage *,
    const struct port_dq_fdt *,const struct port_dq_bootread_request *,
    struct port_dq_bootread_result *,uint32_t);
uint32_t port_dqddt_dispatch_bootread(const struct port_dqddt_stage *,
    const struct port_dq_fdt *,const struct port_dq_bootread_request *,
    struct port_dq_bootread_result *,uint32_t);
uint32_t port_dqddt_dispatch_bootwrite(const struct port_dqddt_stage *,
    const struct port_dq_fdt *,const struct port_dq_bootread_request *,
    struct port_dq_bootread_result *,uint32_t);
/* Partial native DQ_STARTIO entry: dispatches only validated physical
 * READPBLK/WRITEPBLK requests through their DDT FDT actions. */
uint32_t port_dqddt_startio(const struct port_dqddt_stage *,
    const struct port_dqddt_native *,const struct port_dq_fdt *,
    const struct port_dq_bootread_request *,struct port_dq_bootread_result *);
/* Admit one supervisor physical READPBLK through a published native DDT
 * before arming a one-sector asynchronous hardware engine. Completion remains
 * the caller's responsibility so IOC posting can be deferred safely. */
typedef int (*port_dqddt_async_start_fn)(void *,uint32_t,uint16_t [256],
    void (*)(void *,int),void *);
uint32_t port_dqddt_startio_async(const struct port_dqddt_stage *,
    const struct port_dqddt_native *,const struct port_dq_fdt *,
    const struct port_dq_bootread_request *,uint32_t,uint16_t [256],
    port_dqddt_async_start_fn,void (*)(void *,int),void *);
typedef int (*port_dqddt_async_write_start_fn)(void *,uint32_t,
    const uint16_t [256],void (*)(void *,int),void *);
/* Validated native DDT dispatch for one privileged physical WRITEPBLK.
 * Buffer and caller context must remain live through the completion callback. */
uint32_t port_dqddt_startio_async_write(const struct port_dqddt_stage *,
    const struct port_dqddt_native *,const struct port_dq_fdt *,
    const struct port_dq_bootread_request *,uint32_t,
    port_dqddt_async_write_start_fn,void (*)(void *,int),void *,void *);
uint32_t port_dqddt_bootwrite(const struct port_dqddt_stage *,
    const struct port_dq_fdt *,const struct port_dq_bootread_request *,
    struct port_dq_bootread_result *,uint32_t);
uint32_t port_dqddt_entry_offset(unsigned);
uint32_t port_dqddt_fdt_check(const struct port_dqddt_stage *,
    const struct port_dq_fdt *);
#endif
