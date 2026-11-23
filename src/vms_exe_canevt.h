#ifndef VMS_EXE_CANEVT_H
#define VMS_EXE_CANEVT_H
#include <stdint.h>
#include "vms_exe_schevt.h"
/* SYS/SRC/SYSCANEVT.MAR: remove TQE requests at timer IPL. */
struct port_exe_cancel_wake_request {
    uint32_t pid_address,logical_name;
};
uint32_t port_exe_cantim(struct port_exe_timer_queue *queue,
                         const struct port_exe_timer_process *caller,
                         uint32_t request_id,uint32_t access_mode,
                         unsigned *removed);
uint32_t port_exe_canwak(struct port_exe_timer_queue *queue,
                         const struct port_exe_timer_process *caller,
                         const struct port_exe_cancel_wake_request *request,
                         const struct port_exe_timer_ops *ops,
                         unsigned *removed);
#endif
