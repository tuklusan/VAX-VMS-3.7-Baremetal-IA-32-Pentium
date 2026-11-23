#include "vms_iniloa_entry.h"
/* INILOA is a separately linked resident image. INIT must supply the real
 * RPB, protected I/O interface, and discovered IA-32 device descriptors.
 * The bootstrap only loads this image; executing it before the source-
 * ordered initialization chain exists would be an invalid boot. */
uint32_t port_iniloa_entry(const struct port_iniloa_start *p){
    if(!p||p->signature!=PORT_INILOA_SIGNATURE||
       p->version!=PORT_INILOA_VERSION)return PORT_INI_MAP_BAD_ARGUMENT;
    return port_ini_iomap(p->adapters,p->adapter_count,p->boot_io_base,
                          p->driver_ops,p->state);
}
