#include "vms_iopost_swi.h"
/* Called ONLY from IA-32 IDT 0x90 (DPL0). The x86 interrupt gate
 * masks further maskable interrupts while the original REMQUE loop
 * operates. A failed callback is never converted to SS$_NORMAL. */
uint32_t port_iopost_swi_dispatch(struct port_iopost_swi *s,
                                  unsigned interrupt_vector){
    if(!s||interrupt_vector!=PORT_IOPOST_SW_VECTOR)
        return PORT_IOPOST_SW_BAD;
    if(!s->armed||!s->action||!s->guest_entry||
       s->guest_entry<s->lower||s->guest_entry>=s->upper)
        return PORT_IOPOST_SW_UNBOUND;
    if(s->in_service)return PORT_IOPOST_SW_BUSY;
    s->in_service=1u;
    ++s->attempts;
    const uint32_t completed=s->action();
    s->last_status=completed;
    ++s->calls;
    s->in_service=0u;
    /* The bootstrap callback is intentionally a single-use end action:
     * later ordinary process I/O requires real CCB/PCB/AST consumers. */
    s->armed=0u;
    return completed==PORT_IOPOST_SW_OK ? PORT_IOPOST_SW_OK : PORT_IOPOST_SW_CORRUPT;
}
