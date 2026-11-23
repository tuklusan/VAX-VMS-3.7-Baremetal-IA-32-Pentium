#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include "vms_qio_post.h"
#define REQUIRE(x) do{if(!(x)){fprintf(stderr,"qio-post FAIL line %d: %s\n",__LINE__,#x);return 1;}}while(0)
struct irq_counter {unsigned events;uint8_t last;};
static void softint(void *arg,uint8_t level){
    struct irq_counter *irq=arg;irq->events++;irq->last=level;
}
int main(void){
    enum {BASE=0x00342000u,PKT=0x00343000u,HEAD=0x00344000u};
    uint32_t storage[PORT_V37_DEVICE_BYTES/4]={0};
    struct port_v37_device_result d={0};
    REQUIRE(port_v37_devices_prelink((uint8_t*)storage,sizeof storage,BASE,&d)==PORT_V37_DEV_OK);
    uint8_t *nl=((uint8_t*)storage)+PORT_V37_UCB_NL_OFF;
    uint32_t irpwords[4][PORT_V37_IRP_POOL_STRIDE/4u]={{0}};
    uint8_t *irp=(uint8_t *)irpwords;
    struct port_v37_iopost_queue queue={0};
    struct irq_counter irq={0};
    REQUIRE(port_v37_post_init(&queue,irp,sizeof irpwords,PKT,PORT_V37_IRP_POOL_STRIDE,HEAD)==PORT_V37_QIO_OK);
    REQUIRE(port_v37_post_validate(&queue)==PORT_V37_QIO_OK);
    struct port_v37_qio_request r={.post=&queue,.irp=irp,
        .ucb=nl,.irp_va=PKT,.ucb_va=d.ucb[4],.nl_ddb_va=d.ddb[3],
        .irp_size=PORT_V37_IRP_SIZE,.ucb_size=PORT_V37_UCB_SIZE,
        .softint=softint,.softint_arg=&irq};
    REQUIRE(port_v37_irp_prepare(irp,PORT_V37_IRP_SIZE,PKT,d.ucb[4],PORT_SYS85_READVBLK)==PORT_V37_QIO_OK);
    REQUIRE(port_v37_nl_fdt_post(&r)==PORT_SYS85_NORMAL);
    REQUIRE(port_v37_read32(irp+PORT_V37_IRP_MEDIA)==PORT_SYS85_ENDOFFILE);
    REQUIRE(port_v37_read32(irp+PORT_V37_IRP_IOST2)==0);
    REQUIRE(port_v37_read32(nl+PORT_V37_UCB_OPCNT)==1);
    REQUIRE(queue.first==PKT&&queue.last==PKT&&queue.queued==1);
    REQUIRE(port_v37_read32(irp)==HEAD&&port_v37_read32(irp+4)==HEAD);
    REQUIRE(irq.events==1&&irq.last==4);
    REQUIRE(port_v37_post_validate(&queue)==PORT_V37_QIO_OK);
    /* The second request uses EXE$FINISHIO, preserving R1's bytes count. */
    uint8_t *irp2=irp+PORT_V37_IRP_POOL_STRIDE;
    REQUIRE(port_v37_irp_prepare(irp2,PORT_V37_IRP_SIZE,PKT+PORT_V37_IRP_POOL_STRIDE,d.ucb[4],PORT_SYS85_WRITELBLK)==PORT_V37_QIO_OK);
    struct port_v37_qio_request r2=r;r2.irp=irp2;r2.irp_va=PKT+PORT_V37_IRP_POOL_STRIDE;
    REQUIRE(port_v37_qio_finishio(&r2,PORT_SYS85_NORMAL,512u)==PORT_SYS85_NORMAL);
    REQUIRE(port_v37_read32(irp2+PORT_V37_IRP_MEDIA)==1u);
    REQUIRE(port_v37_read32(irp2+PORT_V37_IRP_IOST2)==512u);
    REQUIRE(port_v37_read32(irp)==PKT+PORT_V37_IRP_POOL_STRIDE&&port_v37_read32(irp2+4)==PKT);
    REQUIRE(port_v37_read32(irp2)==HEAD&&queue.first==PKT&&queue.last==PKT+PORT_V37_IRP_POOL_STRIDE&&queue.queued==2);
    REQUIRE(port_v37_read32(nl+PORT_V37_UCB_OPCNT)==2&&irq.events==2);
    REQUIRE(port_v37_post_validate(&queue)==PORT_V37_QIO_OK);
    const struct port_v37_iopost_queue snapshot=queue;
    const uint32_t operations=port_v37_read32(nl+PORT_V37_UCB_OPCNT);
    REQUIRE(port_v37_qio_finishioc(&r,PORT_SYS85_ENDOFFILE)==PORT_V37_QIO_DUP);
    REQUIRE(queue.queued==snapshot.queued&&irq.events==2);
    REQUIRE(port_v37_read32(nl+PORT_V37_UCB_OPCNT)==operations);
    uint32_t old=port_v37_read32(irp+4);
    port_v37_write32(irp+4,0xdead0000u);
    REQUIRE(port_v37_post_validate(&queue)==PORT_V37_QIO_CORRUPT);
    REQUIRE(port_v37_qio_finishio(&r2,1u,2u)==PORT_V37_QIO_CORRUPT);
    port_v37_write32(irp+4,old);
    REQUIRE(port_v37_post_validate(&queue)==PORT_V37_QIO_OK);
    r2.ucb_va^=4u;
    REQUIRE(port_v37_qio_finishio(&r2,1u,0u)==PORT_V37_QIO_BAD);
    r2.ucb_va^=4u;
    r2.nl_ddb_va=d.ddb[1];
    REQUIRE(port_v37_nl_fdt_post(&r2)==PORT_V37_QIO_BAD);
    r2.nl_ddb_va=d.ddb[3];
    uint8_t *irp3=irp+2u*PORT_V37_IRP_POOL_STRIDE;
    REQUIRE(port_v37_irp_prepare(irp3,PORT_V37_IRP_SIZE,
        PKT+2u*PORT_V37_IRP_POOL_STRIDE,d.ucb[4],PORT_SYS85_READVBLK)==PORT_V37_QIO_OK);
    struct port_v37_qio_request r3=r;r3.irp=irp3;
    r3.irp_va=PKT+2u*PORT_V37_IRP_POOL_STRIDE;
    port_v37_write32(irp3+PORT_V37_QIO_IRP_IOSB,0x12345678u);
    irp3[PORT_V37_IRP_RMOD]|=0x40u;
    int16_t ast_count=INT16_MAX;
    REQUIRE(port_v37_qio_abortio(&r3,&ast_count)==PORT_SYS85_NORMAL);
    REQUIRE(port_v37_read32(irp3+PORT_V37_QIO_IRP_IOSB)==0u);
    REQUIRE(!(irp3[PORT_V37_IRP_RMOD]&0x40u)&&ast_count==INT16_MIN);
    REQUIRE(queue.queued==3&&irq.events==3&&irq.last==PORT_V37_IOPOST_IPL);
    uint8_t *irp4=irp+3u*PORT_V37_IRP_POOL_STRIDE;
    REQUIRE(port_v37_irp_prepare(irp4,PORT_V37_IRP_SIZE,
        PKT+3u*PORT_V37_IRP_POOL_STRIDE,d.ucb[4],PORT_SYS85_READVBLK)==PORT_V37_QIO_OK);
    struct port_v37_qio_request r4=r;r4.irp=irp4;
    r4.irp_va=PKT+3u*PORT_V37_IRP_POOL_STRIDE;
    port_v37_write32(irp4+PORT_V37_QIO_IRP_IOSB,0xaabbccddu);
    ast_count=7;
    REQUIRE(port_v37_qio_abortio(&r4,&ast_count)==PORT_SYS85_NORMAL);
    REQUIRE(port_v37_read32(irp4+PORT_V37_QIO_IRP_IOSB)==0u&&ast_count==7);
    REQUIRE(queue.queued==4&&irq.events==4&&port_v37_post_validate(&queue)==PORT_V37_QIO_OK);
    REQUIRE(port_v37_post_init(0,irp,sizeof irpwords,PKT,64,HEAD)==PORT_V37_QIO_BAD);
    REQUIRE(port_v37_post_init(&queue,irp,sizeof irpwords,PKT,32,HEAD)==PORT_V37_QIO_BAD);
    REQUIRE(port_v37_post_init(&queue,irp,sizeof irpwords,PKT,64,PKT+4)==PORT_V37_QIO_BAD);
    REQUIRE(port_v37_post_validate(&queue)==PORT_V37_QIO_OK);
    REQUIRE(port_v37_irp_prepare(irp2,PORT_V37_IRP_SIZE,PKT+PORT_V37_IRP_POOL_STRIDE,d.ucb[4],0u)==PORT_V37_QIO_OK);
    REQUIRE(port_v37_nl_fdt_post(&r2)==PORT_SYS85_BADFUNC);
    REQUIRE(irq.events==4&&queue.queued==4);
    puts("SYSQIOREQ.MAR ABORTIO IOSB/quota and FINISHIOC IRP post FIFO/IPL4 PASS");
    return 0;
}
