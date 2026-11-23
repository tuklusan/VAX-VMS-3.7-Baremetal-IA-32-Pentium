#ifndef VMS_BOO_SHODEV_H
#define VMS_BOO_SHODEV_H
#include <stdint.h>
#include "vms_exe_gettim.h"
/* BOOTS/SRC/SHODEV.MAR: traversal of the active driver and device database. */
struct port_boo_unit {uint16_t number;uint32_t address;const struct port_boo_unit *next;};
struct port_boo_devblock {
    const char *name;const char *driver_name;uint32_t address,crb,idb;
    const struct port_boo_unit *units;
    const struct port_boo_devblock *next;
};
struct port_boo_driver {
    const char *name;uint32_t address,size;
    const struct port_boo_devblock *devices;
    const struct port_boo_driver *next;
};
struct port_boo_shodev {
    void *context;
    const struct port_boo_driver *drivers;
    uint32_t (*change_to_exec)(void *);
    uint32_t (*write_line)(void *,const char *data,uint16_t length);
    void (*signal)(void *,uint32_t);
};
/* Driver mode =1, device mode =0, optional parser token filters by driver. */
uint32_t port_boo_shodev(struct port_boo_shodev *,const char *token,
                          uint16_t token_size,uint8_t driver_mode);
#endif
