#ifndef VMS_V37_SCHED_H
#define VMS_V37_SCHED_H
#include <stddef.h>
#include <stdint.h>
#include "vms_v37_pcb.h"

enum {
    PORT_V37_SCH_PRIORITIES=32u,
    PORT_V37_SCH_QUEUES=64u,
    PORT_V37_SCH_COLPG=1u,PORT_V37_SCH_MWAIT=2u,PORT_V37_SCH_CEF=3u,
    PORT_V37_SCH_PFW=4u,PORT_V37_SCH_LEF=5u,PORT_V37_SCH_LEFO=6u,
    PORT_V37_SCH_HIB=7u,PORT_V37_SCH_HIBO=8u,PORT_V37_SCH_SUSP=9u,
    PORT_V37_SCH_SUSPO=10u,PORT_V37_SCH_FPG=11u,
    PORT_V37_SCH_COM=12u,
    PORT_V37_SCH_COMO=13u,
    PORT_V37_SCH_CUR=14u,
    PORT_V37_SCH_RESIDENT=1u,
    PORT_V37_SCH_INCREMENT_CLASSES=5u
};
enum {PORT_V37_SCHED_OK=1u,PORT_V37_SCHED_BADPARAM=2u,
      PORT_V37_SCHED_CORRUPT=4u,PORT_V37_SCHED_UNRESOLVED=8u,
      PORT_V37_SCHED_EMPTY=16u,PORT_V37_SCHED_NO_CONTEXT=32u,
      PORT_V37_SCHED_BLOCKED=64u,PORT_V37_SCHED_IGNORED=128u,PORT_V37_SCHED_UNASSIGNED=256u};
enum {PORT_V37_RSE_AST=0u,PORT_V37_RSE_EVENT_LEF=1u,
      PORT_V37_RSE_EVENT_CEF=2u,
      PORT_V37_RSE_EVENT_LEFO=3u,PORT_V37_RSE_FPGA=4u,PORT_V37_RSE_WAKE=5u,
      PORT_V37_RSE_RESUME=6u,PORT_V37_RSE_PFCOM=7u,
      PORT_V37_RSE_SETPRI=8u};
enum {PORT_V37_CEB_BYTES=56u,PORT_V37_CEB_TYPE=10u,
      PORT_V37_CEB_EFC=16u,PORT_V37_CEB_WQH=20u,
      PORT_V37_CEB_NORMAL=4u,PORT_V37_CEB_SLAVE=45u,
      PORT_V37_CEB_SHARED=46u,PORT_V37_SS_ILLEFC=236u,
      PORT_V37_SS_UNASEFC=564u,PORT_V37_SS_WASSET=9u};

struct port_v37_sched_head {uint32_t forward,backward;};
/* SYSDEFQZ.MDL WQH: forward/back links, 16-bit count and wait state. */
struct port_v37_wait_queue {
    uint32_t forward,backward;
    uint16_t count,state;
};
struct port_v37_sched_ops {
    void *context;
    uint32_t (*address)(void *,const void *);
    uint8_t *(*resolve)(void *,uint32_t,size_t);
    void (*wake_swapper)(void *,uint32_t);
    void (*request_reschedule)(void *);
    /* SYSWAIT/POSTEF enter the source IPL_SYNCH critical section before
     * examining shared event flags or mutating WQH/scheduler state. */
    uint32_t (*enter_synchronization)(void *);
    void (*leave_synchronization)(void *,uint32_t);
};
struct port_v37_scheduler {
    struct port_v37_sched_head heads[PORT_V37_SCH_QUEUES];
    uint32_t summary[2];
    uint32_t current_pcb,current_priority;
    uint32_t ready_count[PORT_V37_SCH_PRIORITIES];
    struct port_v37_sched_ops ops;
    uint32_t initialized;
    uint32_t native_interrupt_count,native_no_context_count;
};
typedef uint32_t (*port_v37_sched_interrupt_fn)(struct port_v37_scheduler *,
    uint32_t,uint32_t *,uint32_t *);

/* SYS/SRC/SDAT.MAR resident/nonresident COM queue headers and SCH$GL_COMQS. */
uint32_t port_v37_sched_init(struct port_v37_scheduler *,
    const struct port_v37_sched_ops *);
/* Resident and nonresident queue portion of RSE.MAR SCH$CHSE/SCH$CHSEP.
 * Arbitrary wait-queue owners and swapper wake semantics remain callbacks. */
uint32_t port_v37_sched_chse(struct port_v37_scheduler *,uint32_t pcb_address,
    uint8_t priority_increment_class);
/* SCH$CHSEP precomputed-priority queue transition for COM/COMO processes. */
uint32_t port_v37_sched_set_priority(struct port_v37_scheduler *,
    uint32_t pcb_address,uint8_t priority);
/* SCH$SCHED queue selection through CURPCB/priority bookkeeping. Refuses
 * to consume a runnable PCB until real PHD and IA-32 PHYPCB context exist. */
uint32_t port_v37_sched_select(struct port_v37_scheduler *,uint32_t *pcb_address,
    uint32_t *ia32_restore_frame);
/* SCH$RESCHED queue insertion after an architecture caller saves context. */
uint32_t port_v37_sched_resched_current(struct port_v37_scheduler *,
    uint32_t current_pcb_address);
/* Save and validate the interrupted native IRET frame in PCB$L_PHYPCB and
 * PHD KSP/PC, then perform SCH$RESCHED's resident queue insertion. */
uint32_t port_v37_sched_save_current(struct port_v37_scheduler *,
    uint32_t current_restore_frame_address);
/* Interrupt-level SCH$RESCHED then SCH$SCHED transaction. Caller holds the
 * scheduler IPL; a contextless current/ready process is left unconsumed. */
uint32_t port_v37_sched_interrupt(struct port_v37_scheduler *,
    uint32_t interrupted_restore_frame_address,uint32_t *selected_pcb,
    uint32_t *selected_restore_frame);
/* SYSWAIT.MAR SCH$WAITK queue/state/context transition. BLOCKED means the
 * process was committed to its WQH but no runnable context could be selected;
 * it is never a retry-safe failure. Quota/time charging, pending-AST delivery
 * and the higher-level event-flag wait remain separate. */
uint32_t port_v37_sched_wait_current(struct port_v37_scheduler *,
    uint32_t wait_queue_address,uint32_t interrupted_restore_frame_address,
    uint32_t *selected_pcb,uint32_t *selected_restore_frame);
/* Remove one process from a source WQH and make it eligible through CHSE. */
uint32_t port_v37_sched_wake_waiter(struct port_v37_scheduler *,
    uint32_t wait_queue_address,uint32_t pcb_address,
    uint8_t priority_increment_class);
/* Partial SCH$RSE event/state dispatch for AST, event flags, FPGA, WAKE,
 * RESUME and PFCOM. AST queue delivery and other source events are unresolved. */
/* For SETPRI, priority_class carries the requested priority 0..31; other
 * executable-state events use it as SCH$CHSE's increment class. */
uint32_t port_v37_sched_rse_event(struct port_v37_scheduler *,uint8_t event,
    uint32_t wait_queue_address,uint32_t pcb_address,uint8_t priority_class);
/* SYSWAIT.MAR WFLOR/WFLAND local and common cluster wait. Event flags and
 * WQH/CEB records are SYSDEF layouts. The scheduler requires synchronization
 * callbacks; *service_status is valid only when it returns OK. */
uint32_t port_v37_sched_wait_event(struct port_v37_scheduler *,
    uint32_t local_lef_wqh,uint32_t interrupted_frame,uint32_t efn,
    uint32_t mask,int wait_all,uint32_t *selected_pcb,
    uint32_t *selected_frame,uint32_t *service_status);
/* SYSEVTSRV.MAR SETEF/POSTEF: set one event bit, evaluate source EFWM/WALL
 * conditions, and wake matching WQH waiters through RSE/CHSE. */
uint32_t port_v37_sched_post_event(struct port_v37_scheduler *,
    uint32_t target_pcb,uint32_t local_lef_wqh,uint32_t efn,
    uint8_t priority_class,uint32_t *service_status);
uint32_t port_v37_sched_check(const struct port_v37_scheduler *);
#endif
