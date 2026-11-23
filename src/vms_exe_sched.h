#ifndef VMS_EXE_SCHED_H
#define VMS_EXE_SCHED_H
#include <stdint.h>
#include "vms_exe_gettim.h"
/* SYS/SRC/SCHED.MAR: 32 priority queues, VAX inverted priority order. */
enum { PORT_EXE_STATE_READY=1u,
       PORT_EXE_STATE_CURRENT=2u,PORT_EXE_STATE_OTHER=3u,
       PORT_EXE_SCHED_PRIORITIES=32u };
struct port_exe_sched_process {
    struct port_exe_sched_process *next,*previous;
    uint32_t pid;
    uint8_t type,base_priority,current_priority,state;
};
struct port_exe_scheduler {
    struct port_exe_sched_process *head[PORT_EXE_SCHED_PRIORITIES];
    struct port_exe_sched_process *tail[PORT_EXE_SCHED_PRIORITIES];
    struct port_exe_sched_process *current;
    uint32_t ready_mask;
    uint8_t current_priority,pcb_type;
};
struct port_exe_sched_ops {
    void *context;
    /* Each callback carries out a real CPU or kernel queue operation. */
    void (*save_context)(void *,struct port_exe_sched_process *);
    void (*load_context)(void *,struct port_exe_sched_process *);
    void (*idle_until_interrupt)(void *);
    void (*fatal_bad_queue)(void *);
};
void port_exe_sched_init(struct port_exe_scheduler *s,uint8_t pcb_type);
uint32_t port_exe_sched_enqueue(struct port_exe_scheduler *s,
                               struct port_exe_sched_process *p);
uint32_t port_exe_sched_resched(struct port_exe_scheduler *s,
                               const struct port_exe_sched_ops *ops);
uint32_t port_exe_sched_select(struct port_exe_scheduler *s,
                              const struct port_exe_sched_ops *ops);
#endif
