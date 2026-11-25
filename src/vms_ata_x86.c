#include <stdint.h>
#include "vms_ata.h"
#include "vms_ata_x86.h"
/* Native x86 primary-master PIO with real PIC IRQ14 sector completion. */
extern int arch_disk_irq_bind(void (*handler)(void));
enum { ATA_STATUS=0x1f7, ATA_COMMAND=0x1f7, ATA_BUSY=0x80, ATA_DRQ=0x08,
       ATA_ERROR=0x01, ATA_DF=0x20, ATA_IRQ_SPIN_LIMIT=100000000u,
       ATA_WRITE_POLL_LIMIT=10000000u };
static volatile uint32_t irq_armed,irq_done,irq_write_phase;
static volatile uint32_t irq_completed;
static volatile int irq_result;
static volatile uint32_t irq_async_read;
static uint16_t *volatile async_read_destination;
static const uint16_t *volatile async_write_source;
static port_ata_x86_completion_fn volatile async_read_completion;
static void *volatile async_read_context;
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
    if(irq_async_read){
        int result;
        if(status==0u||status==0xffu)result=PORT_ATA_NO_DISK;
        else if(status&(ATA_ERROR|ATA_DF))result=PORT_ATA_DEVICE_ERROR;
        else if((status&ATA_BUSY)||!(status&ATA_DRQ))
            result=PORT_ATA_DEVICE_ERROR;
        else{
            uint16_t *destination=async_read_destination;
            if(!destination)result=PORT_ATA_BAD_ARG;
            else{
                for(unsigned i=0u;i<256u;i++)
                    destination[i]=read16(0,0x1f0u);
                result=PORT_ATA_OK;
            }
        }
        port_ata_x86_completion_fn completion=async_read_completion;
        void *context=async_read_context;
        async_read_destination=0;async_read_completion=0;
        async_read_context=0;irq_async_read=0;irq_armed=0;
        __asm__ volatile("" ::: "memory");
        ++irq_completed;irq_result=result;irq_done=1u;
        if(completion)completion(context,result);
        return;
    }
    if(async_write_source){
        int result;
        if(status==0u||status==0xffu)result=PORT_ATA_NO_DISK;
        else if(status&(ATA_ERROR|ATA_DF))result=PORT_ATA_DEVICE_ERROR;
        else if((status&ATA_BUSY)||(status&ATA_DRQ))
            result=PORT_ATA_DEVICE_ERROR;
        else result=PORT_ATA_OK;
        port_ata_x86_completion_fn completion=async_read_completion;
        void *context=async_read_context;
        async_write_source=0;async_read_completion=0;async_read_context=0;
        irq_armed=0;__asm__ volatile("" ::: "memory");
        ++irq_completed;irq_result=result;irq_done=1u;
        if(completion)completion(context,result);
        return;
    }
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
    irq_armed=irq_done=irq_write_phase=irq_async_read=0;
    async_read_destination=0;async_write_source=0;
    async_read_completion=0;async_read_context=0;
    irq_completed=0;
    irq_result=PORT_ATA_TIMEOUT;
    write8(0,0x3f6,0); /* enable device interrupts on the primary channel */
    for(unsigned i=0;i<4u;i++)(void)read8(0,0x3f6);
    return arch_disk_irq_bind(port_ata_x86_irq);
}
uint32_t port_ata_x86_irq_count(void){return irq_completed;}
int port_ata_x86_read28_async(uint32_t lba,uint16_t destination[256],
    port_ata_x86_completion_fn completion,void *context){
    if(lba>0x0fffffffu||!destination||!completion)
        return PORT_ATA_X86_ASYNC_BAD;
    if(irq_armed||irq_async_read)return PORT_ATA_X86_ASYNC_BUSY;
    write8(0,0x1f6u,(uint8_t)(0xe0u|((lba>>24u)&15u)));
    for(unsigned i=0u;i<4u;i++)(void)read8(0,0x3f6u);
    uint8_t status=0u;
    for(uint32_t i=0u;i<100000u;i++){
        status=read8(0,ATA_STATUS);
        if(!status||status==0xffu)return PORT_ATA_NO_DISK;
        if(status&(ATA_ERROR|ATA_DF))return PORT_ATA_DEVICE_ERROR;
        if(!(status&ATA_BUSY))break;
        if(i==99999u)return PORT_ATA_TIMEOUT;
    }
    async_read_destination=destination;
    async_read_completion=completion;async_read_context=context;
    irq_done=0u;irq_result=PORT_ATA_TIMEOUT;irq_async_read=1u;
    __asm__ volatile("" ::: "memory");irq_armed=1u;
    write8(0,0x1f2u,1u);
    write8(0,0x1f3u,(uint8_t)lba);
    write8(0,0x1f4u,(uint8_t)(lba>>8u));
    write8(0,0x1f5u,(uint8_t)(lba>>16u));
    write8(0,ATA_COMMAND,0x20u);
    return PORT_ATA_X86_ASYNC_OK;
}
int port_ata_x86_write28_async(uint32_t lba,const uint16_t source[256],
    port_ata_x86_completion_fn completion,void *context){
    if(lba>0x0fffffffu||!source||!completion)
        return PORT_ATA_X86_ASYNC_BAD;
    if(irq_armed||irq_async_read||async_write_source)
        return PORT_ATA_X86_ASYNC_BUSY;
    write8(0,0x1f6u,(uint8_t)(0xe0u|((lba>>24u)&15u)));
    for(unsigned i=0u;i<4u;i++)(void)read8(0,0x3f6u);
    uint8_t status=0u;
    for(uint32_t i=0u;i<100000u;i++){
        status=read8(0,ATA_STATUS);
        if(!status||status==0xffu)return PORT_ATA_NO_DISK;
        if(status&(ATA_ERROR|ATA_DF))return PORT_ATA_DEVICE_ERROR;
        if(!(status&ATA_BUSY))break;
        if(i==99999u)return PORT_ATA_TIMEOUT;
    }
    write8(0,0x1f2u,1u);
    write8(0,0x1f3u,(uint8_t)lba);
    write8(0,0x1f4u,(uint8_t)(lba>>8u));
    write8(0,0x1f5u,(uint8_t)(lba>>16u));
    write8(0,ATA_COMMAND,0x30u);
    for(uint32_t i=0u;i<ATA_WRITE_POLL_LIMIT;i++){
        status=read8(0,ATA_STATUS);
        if(!status||status==0xffu)return PORT_ATA_NO_DISK;
        if(status&(ATA_ERROR|ATA_DF))return PORT_ATA_DEVICE_ERROR;
        if(!(status&ATA_BUSY)&&(status&ATA_DRQ))break;
        if(i==ATA_WRITE_POLL_LIMIT-1u)return PORT_ATA_TIMEOUT;
    }
    uint32_t flags;
    __asm__ volatile("pushfl; popl %0; cli":"=r"(flags)::"memory");
    async_write_source=source;async_read_completion=completion;
    async_read_context=context;irq_done=0u;irq_result=PORT_ATA_TIMEOUT;
    __asm__ volatile("" ::: "memory");irq_armed=1u;
    for(unsigned i=0u;i<256u;i++)write16(0,0x1f0u,source[i]);
    __asm__ volatile("" ::: "memory");
    if(flags&UINT32_C(0x200))__asm__ volatile("sti" ::: "memory");
    return PORT_ATA_X86_ASYNC_OK;
}
int port_ata_x86_read28(uint32_t lba,uint16_t dst[256]) {
    const struct port_ata_bus bus={read8,read16,write8,0,begin_read,wait_read};
    return port_ata_read28(&bus,lba,dst);
}
int port_ata_x86_write28(uint32_t lba,const uint16_t src[256]) {
    const struct port_ata_write_bus bus={read8,write8,write16,0,begin_write,
        wait_write_data,arm_write_complete,wait_write_complete};
    return port_ata_write28(&bus,lba,src);
}
