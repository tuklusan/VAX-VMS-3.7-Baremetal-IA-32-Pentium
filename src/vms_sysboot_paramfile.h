#ifndef VMS_SYSBOOT_PARAMFILE_H
#define VMS_SYSBOOT_PARAMFILE_H
#include <stddef.h>
#include <stdint.h>
#include "vms_ini_sysparams.h"
#include "vms_boot_files11.h"

enum { PORT_SYSBOOT_USEFILE_OK=1u, PORT_SYSBOOT_USEFILE_BAD=2u,
       PORT_SYSBOOT_USEFILE_SMALL=4u, PORT_SYSBOOT_USEFILE_IO=6u,
       PORT_SYSBOOT_USEFILE_PARAMETERS=8u,
       PORT_SYSBOOT_USEFILE_NOT_FOUND=10u };

/* BOO$USEFILE file-open/read/merge boundary. This reads the actual named
 * Files-11 file and applies supported native INIT SYSPARAM descriptors.
 * The complete BOO$SEARCH descriptor table and command interpreter remain
 * outside this routine. The file buffer is caller-owned boot memory. */
uint32_t port_sysboot_usefile(const struct boo_io_driver *,uint32_t,
    const struct port_f11_volume *,const char *,
    struct port_vms37_init_params *,uint8_t startup_file[32],
    uint32_t *applied_count,uint8_t *file_buffer,size_t file_capacity,
    size_t *file_bytes);
#endif
