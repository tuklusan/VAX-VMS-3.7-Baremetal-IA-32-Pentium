#ifndef VMS_EXE_EVENT_H
#define VMS_EXE_EVENT_H
#include <stdint.h>
#include "vms_exe_gettim.h"

/* SYS/SRC/SYSEVTSRV.MAR: clusters 0,1 local; 2,3 shared. */
enum { PORT_SS_WASCLR=PORT_SS_NORMAL };
struct port_exe_common_event {
    uint32_t mask;
    struct port_exe_common_event *master;
    uint8_t slave;
};
struct port_exe_event_process {
    uint32_t pid;
    uint32_t local[2];
    struct port_exe_common_event *common[2];
};
typedef int (*port_exe_event_write)(void *,uint32_t,const uint32_t *);
typedef uint32_t (*port_exe_event_post_fn)(void *,uint32_t,uint32_t,uint32_t);
void port_exe_event_init(struct port_exe_event_process *process,uint32_t pid);
uint32_t port_exe_getefc(struct port_exe_event_process *process,
                         uint32_t efn, volatile uint32_t **mask);
uint32_t port_exe_clref(struct port_exe_event_process *process,uint32_t efn);
uint32_t port_exe_readef(struct port_exe_event_process *process,uint32_t efn,
                         uint32_t result_address,port_exe_event_write write,
                         void *context);
uint32_t port_exe_setef(struct port_exe_event_process *process,uint32_t efn,
                        uint32_t io_priority,port_exe_event_post_fn post,
                        void *context);
/* Internal scheduler update used by SCH$POSTEF. */
uint32_t port_exe_event_post(struct port_exe_event_process *process,
                             uint32_t efn);
#endif
