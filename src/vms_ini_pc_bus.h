#ifndef VMS_INI_PC_BUS_H
#define VMS_INI_PC_BUS_H
#include <stdint.h>
#include "vms_iniloa_iomap.h"
/* Native successor to INITADP.MAR's CPU_NEXUSTABLE/INIT_ROUTINES.
 * On IA-32, PIO adapter resources are controlled by ring-0 I/O privilege,
 * not the VAX SBI physical-nexus page mapping. The boot disk must exist. */
enum {PORT_INI_PC_ATA_PIO=0x101u, PORT_INI_PC_16550_UART=0x102u};
struct port_ini_pc_bus {
    void *context;
    int (*ata_read_sector)(void *,uint32_t,uint16_t [256]);
    int (*uart_ready)(void *);
    uint32_t registered_mask;
    uint32_t probe_count;
    uint32_t register_count;
    uint16_t ata_io_port;
    uint16_t uart_io_port;
};
uint32_t port_ini_pc_describe(uint16_t ata_base,uint16_t uart_base,
                        struct port_ini_io_descriptor devices[2]);
uint32_t port_ini_pc_probe(void *ctx,const struct port_ini_io_descriptor *device);
uint32_t port_ini_pc_register(void *ctx,const struct port_ini_io_descriptor *device);
#endif
