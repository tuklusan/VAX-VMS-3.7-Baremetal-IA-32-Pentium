#ifndef VMS_INILOA_ENTRY_H
#define VMS_INILOA_ENTRY_H
#include <stdint.h>
#include "vms_iniloa_iomap.h"
#define PORT_INILOA_SIGNATURE 0x4c494e49u
#define PORT_INILOA_VERSION 1u
struct port_iniloa_start {
    uint32_t signature;
    uint32_t version;
    uint16_t boot_io_base;
    uint16_t adapter_count;
    const struct port_ini_io_descriptor *adapters;
    const struct port_ini_io_ops *driver_ops;
    struct port_ini_io_state *state;
};
uint32_t port_iniloa_entry(const struct port_iniloa_start *request);
#endif
