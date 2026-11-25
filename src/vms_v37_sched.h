#ifndef VMS_V37_SCHED_H
#define VMS_V37_SCHED_H
#include <stddef.h>
#include <stdint.h>
#include "vms_v37_pcb.h"

enum {
    PORT_V37_SCH_PRIORITIES=32u,
    PORT_V37_SCH_QUEUES=64u,
    PORT_V37_SCH_COM=12u,
    PORT_V37_SCH_COMO=13u,
    PORT_V37_SCH_CUR=14u,
    PORT_V37_SCH_RESIDENT=1u,
    PORT_V37_SCH_INCREMENT_CLASSES=5u
};
enum {PORT_V37_SCHED_OK=1u,PORT_V37_SCHED_BADPARAM=2u,
      PORT_V37_SCHED_CORRUPT=4u,PORT_V37_SCHED_UNRESOLVED=8u,
      PORT_V37_SCHED_EMPTY=16u,PORT_V37_SCHED_NO_CONTEXT=32u};

struct port_v37_sched_head {uint32_t forward,backward;};
struct port_v37_sched_ops {
    void *context;
    uint32_t (*address)(void *,const void *);
    uint8_t *(*resolve)(void *,uint32_t,size_t);
    void (*wake_swapper)(void *,uint32_t);
    void (*request_reschedule)(void *);
};
struct port_v37_scheduler {
    struct port_v37_sched_head heads[PORT_V37_SCH_QUEUES];
    uint32_t summary[2];
    uint32_t current_pcb,current_priority;
    uint32_t ready_count[PORT_V37_SCH_PRIORITIES];
    struct port_v37_sched_ops ops;
    uint32_t initialized;
};

/* SYS/SRC/SDAT.MAR resident/nonresident COM queue headers and SCH$GL_COMQS. */
uint32_t port_v37_sched_init(struct port_v37_scheduler *,
    const struct port_v37_sched_ops *);
/* Resident and nonresident queue portion of RSE.MAR SCH$CHSE/SCH$CHSEP.
 * Arbitrary wait-queue owners and swapper wake semantics remain callbacks. */
uint32_t port_v37_sched_chse(struct port_v37_scheduler *,uint32_t pcb_address,
    uint8_t priority_increment_class);
/* SCH$SCHED queue selection through CURPCB/priority bookkeeping. Refuses
 * to consume a runnable PCB until real PHD and IA-32 PHYPCB context exist. */
uint32_t port_v37_sched_select(struct port_v37_scheduler *,uint32_t *pcb_address,
    uint32_t *ia32_restore_frame);
/* SCH$RESCHED queue insertion after an architecture caller saves context. */
uint32_t port_v37_sched_resched_current(struct port_v37_scheduler *,
    uint32_t current_pcb_address);
uint32_t port_v37_sched_check(const struct port_v37_scheduler *);
#endif
