#include <stdint.h>
#include "vms_ata.h"
/* Native x86 primary-master PIO with real PIC IRQ14 sector completion. */
extern int arch_disk_irq_bind(void (*handler)(void));
enum { ATA_STATUS=0x1f7, ATA_BUSY=0x80, ATA_DRQ=0x08,
       ATA_ERROR=0x01, ATA_DF=0x20, ATA_IRQ_SPIN_LIMIT=100000000u,
       ATA_WRITE_POLL_LIMIT=10000000u };
static volatile uint32_t irq_armed,irq_done,irq_write_phase;
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
static void write16(void *ctx,uint16_t port,uint16_t value) {
    (void)ctx;
    __asm__ volatile("outw %w0,%w1"::"a"(value),"Nd"(port));
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
static int begin_write(void *ctx) {
    (void)ctx;
    irq_done=0;irq_result=PORT_ATA_TIMEOUT;irq_write_phase=0u;
    irq_armed=0u;__asm__ volatile("" ::: "memory");
    return 0;
}
static int wait_phase(void) {
    for(uint32_t i=0;i<ATA_IRQ_SPIN_LIMIT;i++) {
        if(irq_done) {
            __asm__ volatile("" ::: "memory");
            return irq_result;
        }
        __asm__ volatile("pause");
    }
    irq_armed=0u;irq_write_phase=0u;
    return PORT_ATA_TIMEOUT;
}
static int wait_write_data(void *ctx) {
    (void)ctx;
    /* ATA PIO exposes the first DRQ phase by status polling. The device IRQ
     * acknowledges completion after the data block has been supplied. */
    for(uint32_t i=0;i<ATA_WRITE_POLL_LIMIT;i++){
        const uint8_t status=read8(0,ATA_STATUS);
        if(!status||status==0xffu)return PORT_ATA_NO_DISK;
        if(status&(ATA_ERROR|ATA_DF))return PORT_ATA_DEVICE_ERROR;
        if(!(status&ATA_BUSY)&&(status&ATA_DRQ)){
            irq_write_phase=2u;
            return PORT_ATA_OK;
        }
        __asm__ volatile("pause");
    }
    return PORT_ATA_TIMEOUT;
}
static int arm_write_complete(void *ctx) {
    (void)ctx;
    if(irq_write_phase!=2u)return -1;
    irq_done=0u;irq_result=PORT_ATA_TIMEOUT;irq_write_phase=3u;
    __asm__ volatile("" ::: "memory");
    irq_armed=1u;
    return 0;
}
static int wait_write_complete(void *ctx) {
    (void)ctx;
    const int rc=wait_phase();
    irq_write_phase=0u;
    return rc;
}
void port_ata_x86_irq(void) {
    uint8_t status=read8(0,ATA_STATUS); /* acknowledges the ATA interrupt */
    if(!irq_armed)return;
    if(status==0u||status==0xffu)irq_result=PORT_ATA_NO_DISK;
    else if(status&(ATA_ERROR|ATA_DF))irq_result=PORT_ATA_DEVICE_ERROR;
    else if((status&ATA_BUSY)||
       (irq_write_phase==1u?!(status&ATA_DRQ):
        irq_write_phase==3u?(status&ATA_DRQ)!=0:
        !(status&ATA_DRQ)))irq_result=PORT_ATA_DEVICE_ERROR;
    else irq_result=PORT_ATA_OK;
    __asm__ volatile("" ::: "memory");
    irq_armed=0;
    if(irq_write_phase==1u)irq_write_phase=2u;
    else if(irq_write_phase==3u)irq_write_phase=0u;
    ++irq_completed;
    irq_done=1;
}
int port_ata_x86_init(void) {
    irq_armed=irq_done=irq_write_phase=0;
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
int port_ata_x86_write28(uint32_t lba,const uint16_t src[256]) {
    const struct port_ata_write_bus bus={read8,write8,write16,0,begin_write,
        wait_write_data,arm_write_complete,wait_write_complete};
    return port_ata_write28(&bus,lba,src);
}
