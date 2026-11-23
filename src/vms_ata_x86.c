#include <stdint.h>
#include "vms_ata.h"
/* Native x86 primary-master PIO with real PIC IRQ14 sector completion. */
extern int arch_disk_irq_bind(void (*handler)(void));
enum { ATA_STATUS=0x1f7, ATA_BUSY=0x80, ATA_DRQ=0x08,
       ATA_ERROR=0x01, ATA_DF=0x20, ATA_IRQ_SPIN_LIMIT=100000000u };
static volatile uint32_t irq_armed,irq_done;
static volatile uint32_t irq_completed;
static volatile int irq_result;
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
static int begin_read(void *ctx) {
    (void)ctx;
    irq_done=0;
    irq_result=PORT_ATA_TIMEOUT;
    irq_armed=1;
    __asm__ volatile("" ::: "memory");
    return 0;
}
static int wait_read(void *ctx) {
    (void)ctx;
    for(uint32_t i=0;i<ATA_IRQ_SPIN_LIMIT;i++) {
        if(irq_done) {
            __asm__ volatile("" ::: "memory");
            return irq_result;
        }
        __asm__ volatile("pause");
    }
    irq_armed=0;
    return PORT_ATA_TIMEOUT;
}
void port_ata_x86_irq(void) {
    uint8_t status=read8(0,ATA_STATUS); /* acknowledges the ATA interrupt */
    if(!irq_armed)return;
    if(status==0u||status==0xffu)irq_result=PORT_ATA_NO_DISK;
    else if(status&(ATA_ERROR|ATA_DF))irq_result=PORT_ATA_DEVICE_ERROR;
    else if((status&ATA_BUSY)||!(status&ATA_DRQ))irq_result=PORT_ATA_DEVICE_ERROR;
    else irq_result=PORT_ATA_OK;
    __asm__ volatile("" ::: "memory");
    irq_armed=0;
    ++irq_completed;
    irq_done=1;
}
int port_ata_x86_init(void) {
    irq_armed=irq_done=0;
    irq_completed=0;
    irq_result=PORT_ATA_TIMEOUT;
    write8(0,0x3f6,0); /* enable device interrupts on the primary channel */
    for(unsigned i=0;i<4u;i++)(void)read8(0,0x3f6);
    return arch_disk_irq_bind(port_ata_x86_irq);
}
uint32_t port_ata_x86_irq_count(void){return irq_completed;}
int port_ata_x86_read28(uint32_t lba,uint16_t dst[256]) {
    const struct port_ata_bus bus={read8,read16,write8,0,begin_read,wait_read};
    return port_ata_read28(&bus,lba,dst);
}
