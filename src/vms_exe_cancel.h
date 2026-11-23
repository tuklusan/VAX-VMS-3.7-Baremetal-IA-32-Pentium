#ifndef VMS_EXE_CANCEL_H
#define VMS_EXE_CANCEL_H
#include "vms_exe_io.h"
/* SYS/SRC/SYSCANCEL.MAR: cancel queued I/O and notify driver. */
uint32_t port_exe_cancel(struct port_exe_io_channel *,uint32_t,
                         const struct port_exe_io_ops *);
uint32_t port_exe_canceln(struct port_exe_io_channel *,uint32_t,uint8_t,
                          const struct port_exe_io_ops *);
#endif
