#ifndef VMS_SYSBOOT_READ_H
#define VMS_SYSBOOT_READ_H
#include <stdint.h>
#include "vms_bootio.h"
#include "vms_boot_files11.h"

/* GETCURRENT from BOOTS/SRC/SYSBOOT.MAR. These offsets are supplied by
 * the linked PC SYS.EXE, just as the original uses the SYS.EXE link values. */
struct port_sysboot_param_location {
    struct boo_image_layout header;
    uint32_t sysparam_image_offset;
    uint32_t sysparam_byte_count;
};
enum { PORT_SYSBOOT_MAX_EXTENTS=96 };
struct port_sysboot_read_result {
    struct port_f11_fid sys_file;
    struct port_f11_extent extents[PORT_SYSBOOT_MAX_EXTENTS];
    uint32_t extent_count;
    uint32_t image_vbn;
    uint32_t last_image_vbn;
    uint32_t sysparam_vbn;
};
enum { PORT_SYSBOOT_BAD_INPUT=2, PORT_SYSBOOT_BAD_IMAGE=4,
       PORT_SYSBOOT_BAD_PARAMETERS=6, PORT_SYSBOOT_SMALL_BUFFER=8 };
/* IMAGE_OPEN, IMAGE_INIT, READ_VIRTUAL and LOAD_CODE from SYSBOOT.MAR.
 * The file must exist on the real Files-11 volume. No image is fabricated. */
uint32_t port_sysboot_image_init(const struct boo_io_driver *disk,
                                 uint32_t read_function,
                                 const struct port_f11_volume *volume,
                                 const char *filespec,
                                 const struct boo_image_layout *layout,
                                 uint8_t file_header[512],
                                 struct port_sysboot_read_result *found);
uint32_t port_sysboot_read_virtual(const struct boo_io_driver *disk,
                                   uint32_t read_function,
                                   const uint8_t file_header[512],
                                   uint32_t start_vbn,uint32_t bytes,
                                   uint8_t *output,uint32_t capacity);
uint32_t port_sysboot_load_code(const struct boo_io_driver *disk,
                                uint32_t read_function,
                                const uint8_t file_header[512],
                                const struct port_sysboot_read_result *image,
                                uint8_t *output,uint32_t capacity,
                                uint32_t *loaded_bytes);
uint32_t port_sysboot_getcurrent(const struct boo_io_driver *disk,
                                 uint32_t read_function,
                                 const struct port_f11_volume *volume,
                                 const char *filespec,
                                 const struct port_sysboot_param_location *loc,
                                 uint8_t *parameters,
                                 uint32_t available,
                                 struct port_sysboot_read_result *found);
#endif
