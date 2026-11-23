#ifndef VMS_EXE_DASSGN_H
#define VMS_EXE_DASSGN_H
#include "vms_exe_io.h"
/* SYS/SRC/SYSDASSGN.MAR: channel release after real I/O completion. */
uint32_t port_exe_dassgn(struct port_exe_io_channel *,uint32_t,
                         const struct port_exe_io_ops *);
#endif
