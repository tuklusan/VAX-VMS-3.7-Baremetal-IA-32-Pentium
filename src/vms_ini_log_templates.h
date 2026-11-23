#ifndef VMS_INI_LOG_TEMPLATES_H
#define VMS_INI_LOG_TEMPLATES_H
#include <stdint.h>
#include "vms_v37_dyn_tags.h"
/* SYS/SRC/INIT.MAR 328-358: prelinked LOG$AL_DISKLOG and SYSDISKLOG.
 * VAX/IA-32 little-endian DYN header, 16-byte padded original records.
 * DYN$C_LOG=11 is source-defined in SHRLIB/SYSDEF.MDL.
 * Real LOG table linkage and boot-volume equivalence are not ready. */
enum {PORT_INI_LOG_TEMPLATE_BYTES=48u,PORT_INI_LOG_SIZE_OFFSET=8u};
void port_ini_log_templates(uint8_t disk[PORT_INI_LOG_TEMPLATE_BYTES],
    uint8_t sysdevice[PORT_INI_LOG_TEMPLATE_BYTES]);
#endif
