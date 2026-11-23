#ifndef VMS37_F11_BOOTPATH_H
#define VMS37_F11_BOOTPATH_H
#include "vms_f11_range.h"
#include "vms_dq_imageprobe.h"
/* Bounded read-only FIL$FINDFILID, FIL$RDCHKFILHDR, FIL$READVBN chain.
 * This acts only on a verified privileged boot volume, never as a process
 * ACP, $OPEN, or $QIO interface. */
enum { PORT_F11_PATH_OK=1u, PORT_F11_PATH_BAD=2u,
       PORT_F11_PATH_MAP=4u, PORT_F11_PATH_IO=6u,
       PORT_F11_PATH_MISSING=8u, PORT_F11_PATH_HEADER=10u };
struct port_f11_boot_file {
    struct port_dq_dir_fid directory, file;
    uint32_t directory_header_lbn,file_header_lbn;
    uint32_t directory_vbn,file_vbn;
    uint8_t header[512];
};
struct port_f11_boot_volume {
    const uint8_t *home,*index,*mfd;
    port_dq_dir_sector_read_fn read_sector;
    void *context;
    uint32_t max_directory_vbn;
};
uint32_t port_f11_boot_validate_name(const char *,size_t);
uint32_t port_f11_boot_header(const struct port_f11_boot_volume *,
    const struct port_dq_dir_fid *,uint32_t *,uint8_t [512]);
uint32_t port_f11_boot_lookup(const struct port_f11_boot_volume *,
    const uint8_t [512],const char *,size_t,uint16_t,
    struct port_dq_dir_fid *,uint32_t *);
uint32_t port_f11_boot_open(const struct port_f11_boot_volume *,
    const char *,size_t,const char *,size_t,uint16_t,
    struct port_f11_boot_file *);
uint32_t port_f11_boot_read(const struct port_f11_boot_volume *,
    const struct port_f11_boot_file *,uint32_t,uint32_t,uint8_t *,size_t,
    uint32_t *);
#endif
