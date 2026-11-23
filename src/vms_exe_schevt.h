#ifndef VMS_EXE_SCHEVT_H
#define VMS_EXE_SCHEVT_H
#include <stdint.h>
#include "vms_exe_event.h"

/* SYS/SRC/SYSSCHEVT.MAR: timed wakeups and event/AST timer requests. */
enum {
    PORT_EXE_TQE_CAPACITY=64u,
    PORT_EXE_TQE_WKSNGL=1u, PORT_EXE_TQE_WKREPT=2u,
    PORT_EXE_TQE_TMSNGL=3u,
    PORT_EXE_TICK_100NS=100000u,
    PORT_SS_EXQUOTA=28u, PORT_SS_INSFMEM=292u,
    PORT_SS_IVLOGNAM=340u, PORT_SS_IVTIME=388u,
    PORT_SS_NONEXPR=2280u
};
/* JIB timer quota is shared by every process in the same VMS job. */
struct port_exe_timer_job {
    uint16_t timer_available;
};
struct port_exe_timer_process {
    uint32_t pid;
    uint16_t ast_available;
    struct port_exe_timer_job *job;
    uint8_t previous_mode;
    uint8_t implicit_wait_disabled;
    struct port_exe_event_process *events;
};
typedef int (*port_exe_timer_read)(void *,uint32_t,void *,uint32_t);
typedef uint32_t (*port_exe_timer_resolve)(void *,uint32_t,uint32_t,
                                            uint32_t,uint32_t *);
typedef uint32_t (*port_exe_timer_signal)(void *,uint32_t);
typedef uint32_t (*port_exe_timer_post)(void *,uint32_t,uint32_t,uint32_t);
typedef uint32_t (*port_exe_timer_ast)(void *,uint32_t,uint32_t,uint32_t,uint8_t);
typedef uint32_t (*port_exe_timer_wait)(void *,uint32_t,uint32_t);
enum { PORT_EXE_WAIT_TIMER_QUOTA=1u, PORT_EXE_WAIT_NONPAGED=2u };
struct port_exe_timer_ops {
    void *context;
    port_exe_timer_read read_user;
    port_exe_timer_resolve resolve_process;
    port_exe_timer_signal wake_process;
    port_exe_timer_post post_event;
    port_exe_timer_ast queue_ast;
    /* Resource wait must actually relinquish the scheduler lock and resume. */
    port_exe_timer_wait wait_resource;
};
struct port_exe_wake_request {
    uint32_t pid_address,logical_name,day_time,repeat_time;
};
struct port_exe_timer_request {
    uint32_t efn,day_time,ast_address,request_id;
};
struct port_exe_timer_entry {
    uint64_t when,interval;
    uint32_t target_pid,requester_pid,ast,ast_parameter;
    struct port_exe_timer_process *owner;
    uint8_t kind,efn,mode,absolute;
};
struct port_exe_timer_queue {
    uint64_t now;
    struct port_exe_timer_entry entries[PORT_EXE_TQE_CAPACITY];
    unsigned used;
    unsigned delivered,failed;
};
void port_exe_timer_init(struct port_exe_timer_queue *queue,uint64_t now);
uint32_t port_exe_schdwk(struct port_exe_timer_queue *queue,
                         struct port_exe_timer_process *caller,
                         const struct port_exe_wake_request *request,
                         const struct port_exe_timer_ops *ops);
uint32_t port_exe_setimr(struct port_exe_timer_queue *queue,
                         struct port_exe_timer_process *caller,
                         const struct port_exe_timer_request *request,
                         const struct port_exe_timer_ops *ops);
/* Process expired timers outside IRQ context, after clock advancement. */
unsigned port_exe_timer_advance(struct port_exe_timer_queue *queue,
                               uint64_t now,const struct port_exe_timer_ops *ops);
#endif
