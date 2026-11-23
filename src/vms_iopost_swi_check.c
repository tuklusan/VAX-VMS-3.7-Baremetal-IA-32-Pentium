#include "vms_iopost_swi.h"
/* Detect missing, duplicate or improperly nested interrupt handling.
 * The exact one-shot original kernel-action completion is observable. */
uint32_t port_iopost_swi_check(const struct port_iopost_swi *s,
    uint32_t required_calls,uint32_t expected_status){
    if(!s||!s->action||!s->guest_entry||!s->lower||
       s->guest_entry<s->lower||s->guest_entry>=s->upper||
       s->in_service||s->armed||s->calls!=required_calls||
       s->attempts!=required_calls||s->last_status!=expected_status)
        return PORT_IOPOST_SW_CORRUPT;
    return PORT_IOPOST_SW_OK;
}
