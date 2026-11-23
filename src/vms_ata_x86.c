#include <stdint.h>
#include "vms_ata.h"
/* Native x86 I/O ports, CPL0 only. Polling PIO intentionally avoids IRQ14
 * until the PIC/interrupt-level executive is more mature. */
static uint8_t read8(void *ctx,uint16_t port) {
    (void)ctx;
    uint8_t value;
    __asm__ volatile("inb %w1,%b0":"=a"(value):"Nd"(port));
    return value;
}
static uint16_t read16(void *ctx,uint16_t port) {
    (void)ctx;
    uint16_t value;
    __asm__ volatile("inw %w1,%w0":"=a"(value):"Nd"(port));
    return value;
}
static void write8(void *ctx,uint16_t port,uint8_t value) {
    (void)ctx;
    __asm__ volatile("outb %b0,%w1"::"a"(value),"Nd"(port));
}
int port_ata_x86_read28(uint32_t lba,uint16_t dst[256]) {
    const struct port_ata_bus bus={read8,read16,write8,0};
    return port_ata_read28(&bus,lba,dst);
}
