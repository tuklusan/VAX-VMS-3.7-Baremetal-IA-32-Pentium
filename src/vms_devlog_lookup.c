#include "vms_devlog_search.h"
/* IOSUBPAGD 200$-330$: use the unchanged source DDB/UCB physical walk.
 * A resolved logical name is not a usable channel until its DDT is live. */
uint32_t port_devlog_lookup(const uint8_t *m,size_t n,uint32_t base,
    const struct port_v37_device_result *dev,unsigned mode,
    struct port_devlog_trace *trace){
    if(!trace||!trace->length)return PORT_V37_IOC_BAD;
    struct port_v37_ioc_result ucb={0};
    const uint32_t status=port_v37_ioc_search(m,n,base,dev,
        trace->physical,trace->length,mode,&ucb);
    trace->ioc_status=status;
    if(status==PORT_V37_IOC_OK||status==PORT_V37_IOC_UNBOUND){
        trace->device=ucb;
        trace->driver_bound=ucb.driver_bound;
        trace->concealed=ucb.concealed;
    }
    return status;
}
