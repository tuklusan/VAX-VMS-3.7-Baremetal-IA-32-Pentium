#include "vms_dqddt.h"
/* DPTAB defaults come straight from DQDRIVER.MAR 392-436; the
 * additional original UCB/SVP/error paths are still unresolved. */
uint32_t port_dqddt_stage_init(struct port_dqddt_stage *s,
    const struct port_dq_fdt *fdt,uint32_t base,uint32_t start,uint32_t ret,
    uint32_t read_action){
    if(port_dqddt_preflight(s,fdt,base,start,ret)!=PORT_DQDDT_OK)
        return PORT_DQDDT_BAD;
    struct port_dqddt_stage tmp={0};
    tmp.dpt.max_units=4u;tmp.dpt.default_units=4u;
    tmp.dpt.buffer_bytes=512u;tmp.dpt.fork_ipl=8u;
    tmp.dpt.device_ipl=21u;tmp.dpt.retry_max=8u;
    tmp.dpt.svp_required=1u;
    if(port_dqddt_fill_header(tmp.image,sizeof tmp.image,base,start,ret)!=PORT_DQDDT_OK||
       port_dqddt_fill_fdt(tmp.image,sizeof tmp.image,fdt,base,read_action)!=PORT_DQDDT_OK)
        return PORT_DQDDT_BAD;
    tmp.table_base=base;tmp.native_start_pc=start;tmp.ioc_return_pc=ret;
    tmp.native_read_action_pc=read_action;
    tmp.ready=1u;tmp.driver_bound=0u;
    *s=tmp;
    return PORT_DQDDT_OK;
}
