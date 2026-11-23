#ifndef VMS_EXE_WAIT_H
#define VMS_EXE_WAIT_H
#include <stdint.h>
#include "vms_exe_event.h"
/* SYS/SRC/SYSWAIT.MAR: WFLOR, WFLAND, WAITFR and scheduler wait path. */
struct port_exe_wait_process {
    struct port_exe_event_process *events;
    uint32_t pid, inverted_wait_mask;
    uint8_t wait_cluster, wait_all, waiting;
    uint16_t direct_io_available, direct_io_limit;
    uint16_t buffered_io_available, buffered_io_limit;
    uint8_t pending_ast_mode, previous_mode, interrupt_level;
};
struct port_exe_wait_ops {
    void *context;
    /* Entry, exit and atomic sleep share the scheduler lock. */
    void (*lock)(void *);
    void (*unlock)(void *);
    uint32_t (*sleep_locked)(void *,struct port_exe_wait_process *);
    void (*wake_swapper)(void *);
    void (*report_ast)(void *,struct port_exe_wait_process *);
};
uint32_t port_exe_waitfr(struct port_exe_wait_process *,uint32_t,
                        const struct port_exe_wait_ops *);
uint32_t port_exe_wflor(struct port_exe_wait_process *,uint32_t,uint32_t,
                       const struct port_exe_wait_ops *);
uint32_t port_exe_wfland(struct port_exe_wait_process *,uint32_t,uint32_t,
                        const struct port_exe_wait_ops *);
#endif
