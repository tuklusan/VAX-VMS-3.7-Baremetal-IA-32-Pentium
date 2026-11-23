#include "vms_iopost_swi.h"
/* No fake ISR pre-registration; bootstrap must validate the SYS.EXE
 * address after the Files-11 image loader has installed it. */
void port_iopost_swi_init(struct port_iopost_swi *s){
    if(!s)return;
    s->action=0;s->guest_entry=0u;s->lower=0u;s->upper=0u;
    s->calls=0u;s->attempts=0u;s->last_status=0u;
    s->in_service=0u;s->armed=0u;
}
