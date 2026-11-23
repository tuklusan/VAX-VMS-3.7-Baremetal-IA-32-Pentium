#ifndef VMS_EXE_VERSION_H
#define VMS_EXE_VERSION_H
#include <stdint.h>
/* SYS/SRC/VERSION.MAR and SYS$GQ_VERSION in SYSCOMMON.MAR. */
extern const uint8_t port_exe_version_quadword[8];
extern const char port_exe_announce[];
extern const uint32_t port_exe_announce_length;
#endif
