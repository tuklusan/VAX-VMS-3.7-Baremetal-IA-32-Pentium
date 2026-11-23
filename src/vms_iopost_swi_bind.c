#include "vms_iopost_swi.h"
/* Only a resident supervisor SYS.EXE entry may own the native vector.
 * Never accept process space, a VAX function pointer, or an alias outside
 * the image loader's independently verified executable range. */
uint32_t port_iopost_swi_bind(struct port_iopost_swi *s,
    port_iopost_swi_action action,uint32_t address,
    uint32_t lower,uint32_t upper){
    if(!s||!action||!address||!lower||lower>=upper ||
       address<lower||address>=upper||s->armed||s->action||
       s->attempts||s->in_service||s->calls)
        return PORT_IOPOST_SW_BAD;
    s->action=action;s->guest_entry=address;
    s->lower=lower;s->upper=upper;
    s->armed=1u;
    return PORT_IOPOST_SW_OK;
}
