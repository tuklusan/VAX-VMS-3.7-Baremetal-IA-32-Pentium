#ifndef VMS_EXE_DCLEXH_H
#define VMS_EXE_DCLEXH_H
#include <stdint.h>
#include "vms_exe_access.h"
#include "vms_exe_gettim.h"
enum { PORT_SS_NOHANDLER=2296u,PORT_SS_IVSSRQ=372u };
/* SYS/SRC/SYSDCLEXH.MAR: exit-control block lists by VMS mode. */
struct port_exe_exit_handlers {
    uint32_t head[4];
};
struct port_exe_dclexh_ops {
    const struct port_exe_access *memory;
    void *context;
    void (*bugcheck_invalid_service)(void *);
};
uint32_t port_exe_dclexh(struct port_exe_exit_handlers *,uint8_t mode,
                         uint32_t block,const struct port_exe_dclexh_ops *);
uint32_t port_exe_canexh(struct port_exe_exit_handlers *,uint8_t mode,
                         uint32_t block,const struct port_exe_dclexh_ops *);
#endif
