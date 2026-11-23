#ifndef VMS_BOO_T58BOOTIO_H
#define VMS_BOO_T58BOOTIO_H
#include <stdint.h>
#include "vms_exe_gettim.h"
/* BOOTS/SRC/T58BOOTIO.MAR: physical TU58 block read for the boot source. */
struct port_t58bootio {
    void *context;
    /* A physical TU58 device driver, not a ROM emulator or sample data. */
    uint32_t (*read_block)(void *,uint8_t drive,uint32_t lbn,uint8_t block[512]);
};
uint32_t port_t58_read_lbn(const struct port_t58bootio *device,uint32_t lbn,uint8_t block[512]);
#endif
