#ifndef VMS_EXE_SETPRI_H
#define VMS_EXE_SETPRI_H
#include <stdint.h>
#include "vms_exe_access.h"
#include "vms_exe_gettim.h"
/* VAX scheduler uses 31 - external priority in its PCB fields. */
struct port_exe_priority_process {
    uint32_t pid;
    uint8_t saved_current,saved_base,base,current;
    uint8_t authorized_base;
    uint16_t mutex_count;
    uint8_t executing;
    uint8_t can_set_any_priority;
};
struct port_exe_setpri_request {
    uint32_t pid_address,logical_name,priority,old_address;
};
struct port_exe_priority_ops {
    const struct port_exe_access *memory;
    void *context;
    uint32_t (*resolve)(void *,uint32_t,uint32_t,uint32_t,
                        struct port_exe_priority_process **);
    /* Scheduler must adjust its ready queues after a priority change. */
    void (*reprioritize)(void *,struct port_exe_priority_process *,uint8_t);
};
uint32_t port_exe_setpri(struct port_exe_priority_process *caller,
                         const struct port_exe_setpri_request *request,
                         const struct port_exe_priority_ops *ops);
#endif
