#include "vms_ini_pc_bus.h"
/* The original INITADP INI$IOMAP maps adapter registers, then invokes
 * type-specific setup. IA-32 PIO has no page-table mapping for port space.
 * The native counterpart probes an *actual* registered ATA driver and
 * records port ownership only after a successful probe. */
uint32_t port_ini_pc_describe(uint16_t ata_base,uint16_t uart_base,
                        struct port_ini_io_descriptor devices[2]) {
    if(!devices||ata_base!=0x1f0u ||
       (uart_base && uart_base!=0x3f8u))return 0u;
    devices[0]=(struct port_ini_io_descriptor){
        ata_base,8u,PORT_INI_PC_ATA_PIO,0u,14u};
    if(uart_base){
        devices[1]=(struct port_ini_io_descriptor){
            uart_base,8u,PORT_INI_PC_16550_UART,0u,4u};
        return 2u;
    }
    return 1u;
}
uint32_t port_ini_pc_probe(void *ctx,const struct port_ini_io_descriptor *device){
    struct port_ini_pc_bus *bus=(struct port_ini_pc_bus *)ctx;
    if(!bus||!device)return 0u;
    ++bus->probe_count;
    if(device->adapter_kind==PORT_INI_PC_ATA_PIO &&
       device->io_base==bus->ata_io_port && device->io_count==8u &&
       bus->ata_read_sector) {
        /* LBA 0 is the actual Files-11 physical disk's boot sector. */
        uint16_t words[256];
        return bus->ata_read_sector(bus->context,0u,words)==0 ? 1u:0u;
    }
    if(device->adapter_kind==PORT_INI_PC_16550_UART &&
       device->io_base==bus->uart_io_port && device->io_count==8u &&
       bus->uart_ready)return bus->uart_ready(bus->context) ? 1u:0u;
    return 0u;
}
uint32_t port_ini_pc_register(void *ctx,const struct port_ini_io_descriptor *device){
    struct port_ini_pc_bus *bus=(struct port_ini_pc_bus *)ctx;
    if(!bus||!device)return 0u;
    uint32_t bit=0u;
    if(device->adapter_kind==PORT_INI_PC_ATA_PIO &&
       device->io_base==bus->ata_io_port && device->io_count==8u &&
       bus->ata_read_sector)bit=1u;
    if(device->adapter_kind==PORT_INI_PC_16550_UART &&
       device->io_base==bus->uart_io_port && device->io_count==8u &&
       bus->uart_ready && bus->uart_ready(bus->context))bit=2u;
    if(!bit || (bus->registered_mask&bit))return 0u;
    /* IA-32 IOPL=0 and ring-0 driver registration protect PIO access.
     * No VAX PTE, SBI bus address or fictitious interrupt is installed. */
    bus->registered_mask|=bit;
    ++bus->register_count;
    return 1u;
}
