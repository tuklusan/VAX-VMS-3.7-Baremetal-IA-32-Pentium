#ifndef VMS_EXE_DERLMB_H
#define VMS_EXE_DERLMB_H
#include <stdint.h>
#include "vms_exe_gettim.h"
enum { PORT_SS_DEVALLOC=2112u };
struct port_exe_error_mailbox {
    uint16_t unit;
    uint32_t owner_pid;
};
struct port_exe_erlog_ops {
    void *context;
    /* Synchronize against error-log forwarding and other declarations. */
    void (*lock)(void *);
    void (*unlock)(void *);
};
uint32_t port_exe_derlmb(struct port_exe_error_mailbox *mailbox,
                         uint32_t pid,uint32_t unit,int has_diagnose_privilege,
                         const struct port_exe_erlog_ops *ops);
#endif
