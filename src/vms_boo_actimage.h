#ifndef VMS_BOO_ACTIMAGE_H
#define VMS_BOO_ACTIMAGE_H
#include <stdint.h>
#include "vms_exe_access.h"
#include "vms_exe_gettim.h"
/* BOOTS/SRC/ACTIMAGE.MAR: executive image replacement. */
struct port_boo_actimage_ops {
    void *context;
    const struct port_exe_access *memory;
    uint32_t (*enter_exec)(void *);
    uint32_t (*reset_exec_stack)(void *);
    uint32_t (*rundown_user)(void *);
    /* Must activate the image using PROCSTRT, not announce success. */
    uint32_t (*activate)(void *,const uint8_t *,uint16_t);
};
uint32_t port_boo_actimage(uint32_t name_descriptor,
                           const struct port_boo_actimage_ops *ops);
#endif
