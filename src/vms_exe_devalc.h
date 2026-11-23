#ifndef VMS_EXE_DEVALC_H
#define VMS_EXE_DEVALC_H
#include "vms_exe_io.h"
/* SYS/SRC/SYSDEVALC.MAR: exclusive device allocation and release. */
struct port_exe_dev_request {
    uint32_t name_descriptor,physical_descriptor,physical_length;
    uint32_t access_mode;
};
struct port_exe_device_ops {
    void *context;
    const struct port_exe_access *memory;
    struct port_exe_device **devices;
    unsigned device_count;
    void (*lock_io)(void *);
    void (*unlock_io)(void *);
    uint32_t (*find)(void *,uint32_t,uint8_t,struct port_exe_device **);
    uint32_t (*check_read_access)(void *,struct port_exe_device *,uint32_t);
    uint32_t (*driver_cancel)(void *,struct port_exe_device *);
};
uint32_t port_exe_alloc(struct port_exe_device *,uint32_t pid,uint32_t uic,
                        uint8_t previous_mode,uint8_t can_alloc_spooled,
                        const struct port_exe_dev_request *,
                        const struct port_exe_device_ops *);
uint32_t port_exe_dalloc(uint32_t pid,uint8_t previous_mode,
                         uint32_t name_descriptor,uint32_t requested_mode,
                         const struct port_exe_device_ops *);
#endif
