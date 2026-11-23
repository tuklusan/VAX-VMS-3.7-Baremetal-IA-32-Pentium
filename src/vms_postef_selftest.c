#include "vms_postef.h"
/* Shared host/guest deterministic coverage of POSTEF source responsibilities.
 * Every bit denotes a major, independently testable service invariant.
 */
static uint32_t pd[1024] __attribute__((aligned(4096)));
uint32_t port_postef_selftest(void) {
    struct port_postef_service s;
    struct port_pcb a,b;
    struct port_event_cluster shared;
    uint32_t errors=0;
    unsigned woke=99;
    port_postef_init(&s);
    port_event_init(&shared);
    if (port_pcb_create(&a,0x5151,pd,0x24000,0x10000) ||
        port_pcb_create(&b,0x5152,pd,0x25000,0x10000) ||
        port_pcb_activate(&a) || port_pcb_activate(&b)) return 0x80000000u;
    if (port_postef_register(&s,&a) || port_postef_register(&s,&b) ||
        !port_postef_register(&s,&a) ||
        port_postef_bind_common(&s,2,&shared) ||
        port_postef_attach_common(&s,a.pid,2) ||
        port_postef_attach_common(&s,b.pid,2)) errors|=1u;
    if (port_postef_post(&s,a.pid,0,1,&woke)!=PORT_POSTEF_WAS_CLEAR ||
        woke || port_postef_post(&s,a.pid,0,1,&woke)!=PORT_POSTEF_WAS_SET ||
        port_postef_clear_mask(&s,a.pid,0,1) ||
        port_postef_post(&s,a.pid,0,1,&woke)!=PORT_POSTEF_WAS_CLEAR)
        errors|=2u;
    if (port_postef_wait(&s,a.pid,0,3,0)!=PORT_POSTEF_WAIT_IMMEDIATE ||
        port_postef_clear_mask(&s,a.pid,0,1) ||
        port_postef_wait(&s,a.pid,0,3,0)!=PORT_POSTEF_WAIT_BLOCKED ||
        port_postef_post(&s,a.pid,1,2,&woke)!=PORT_POSTEF_WAS_CLEAR ||
        woke!=1 || a.state!=PORT_PCB_READY) errors|=4u;
    if (port_pcb_activate(&a) ||
        port_postef_wait(&s,a.pid,2,3,0)!=PORT_POSTEF_WAIT_BLOCKED ||
        port_postef_wait(&s,b.pid,2,3,1)!=PORT_POSTEF_WAIT_BLOCKED ||
        port_postef_post(&s,a.pid,64,3,&woke)!=PORT_POSTEF_WAS_CLEAR ||
        woke!=1 || a.state!=PORT_PCB_READY || b.state!=PORT_PCB_WAIT_EF ||
        port_postef_post(&s,a.pid,65,4,&woke)!=PORT_POSTEF_WAS_CLEAR ||
        woke!=1 || b.state!=PORT_PCB_READY || s.wakes!=3)
        errors|=8u;
    if (s.notifications!=2 || port_postef_pending(&s)!=2 ||
        port_postef_drain_one(&s)!=1 || port_postef_drain_one(&s)!=1 ||
        s.replica[0].flags!=3 || s.delivered!=2 ||
        port_postef_drain_one(&s)!=0) errors|=16u;
    for (unsigned i=2;i<10;i++) {
        if (port_postef_post(&s,b.pid,64+i,2,&woke)!=PORT_POSTEF_WAS_CLEAR)
            errors|=32u;
    }
    if (port_postef_pending(&s)!=PORT_POSTEF_SYNC_SLOTS ||
        port_postef_post(&s,b.pid,74,2,&woke)!=PORT_POSTEF_SYNC_BACKLOG ||
        !(shared.flags&(1u<<10)) || s.sync_failures!=1) errors|=32u;
    if (port_postef_post(&s,0xDEAD,1,2,&woke)!=PORT_POSTEF_BAD_PID ||
        port_postef_post(&s,a.pid,128,2,&woke)!=PORT_POSTEF_BAD_CLUSTER ||
        port_postef_post(&s,a.pid,97,2,&woke)!=PORT_POSTEF_UNASSIGNED ||
        port_postef_post_mask(&s,a.pid,2,0,2,&woke)!=PORT_POSTEF_INVALID)
        errors|=64u;
    if (port_pcb_activate(&a) ||
        port_postef_wait(&s,a.pid,0,1u<<5,1)!=PORT_POSTEF_WAIT_BLOCKED ||
        port_postef_unregister(&s,a.pid)!=PORT_POSTEF_INVALID ||
        port_postef_post(&s,a.pid,5,7,&woke)!=PORT_POSTEF_WAS_CLEAR ||
        woke!=1 || port_postef_unregister(&s,a.pid)!=0 ||
        port_postef_unregister(&s,b.pid)!=0 ||
        port_postef_unregister(&s,b.pid)!=PORT_POSTEF_BAD_PID) errors|=128u;
    return errors;
}
