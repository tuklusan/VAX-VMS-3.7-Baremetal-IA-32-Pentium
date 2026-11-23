#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include "vms_iopost_end.h"
#include "vms_nl_ddt.h"
#define CHECK(x) do { if(!(x)) {fprintf(stderr,"IOPOST end FAIL %d: %s\n",__LINE__,#x);return 1;} }while(0)
struct irq {uint32_t count,level;};
static void interrupt(void *v,uint8_t level){struct irq *p=v;p->count++;p->level=level;}
int main(void){
    enum {BASE=0x00342000u,DDT=0x00349000u,RET=0x00320101u,
          READ=0x00320120u,WRITE=0x00320160u,
          PKTS=0x0034a000u,HEAD=0x0034b000u,
          BEGIN=0x00320000u,END=0x00380000u,ACTION=0x00321703u};
    uint32_t dbwords[PORT_V37_DEVICE_BYTES/4u]={0};
    uint8_t *db=(uint8_t*)dbwords;
    struct port_v37_device_result device={0};
    CHECK(port_v37_devices_prelink(db,sizeof dbwords,BASE,&device)==PORT_V37_DEV_OK);
    uint8_t ddt[PORT_V37_NL_IMAGE_BYTES];
    CHECK(port_v37_nl_ddt_build(ddt,sizeof ddt,DDT,RET,READ,WRITE)==PORT_V37_NL_DDT_OK);
    CHECK(port_v37_nl_ddt_bind(db,sizeof dbwords,BASE,&device,DDT)==PORT_V37_NL_DDT_OK);
    uint32_t irp_words[2u][PORT_V37_IRP_SIZE/4u]={{0}};
    struct port_v37_iopost_queue q={0};
    struct port_v37_iopost_endlog log={0};
    struct irq events={0};
    uint8_t *pool=(uint8_t*)irp_words;
    CHECK(port_v37_post_init(&q,pool,sizeof irp_words,PKTS,PORT_V37_IRP_SIZE,HEAD)==PORT_V37_QIO_OK);
    const uint16_t funcs[2]={PORT_SYS85_READVBLK,PORT_SYS85_WRITELBLK};
    for(uint32_t i=0;i<2u;i++){
        uint8_t *pkt=pool+i*PORT_V37_IRP_SIZE;
        CHECK(port_v37_irp_prepare(pkt,PORT_V37_IRP_SIZE,
             PKTS+i*PORT_V37_IRP_SIZE,device.ucb[4],funcs[i])==PORT_V37_QIO_OK);
        port_v37_write32(pkt+PORT_V37_IRP_PID,ACTION);
        struct port_v37_qio_request req={
            .post=&q,.irp=pkt,.ucb=db+PORT_V37_UCB_NL_OFF,
            .irp_va=PKTS+i*PORT_V37_IRP_SIZE,.ucb_va=device.ucb[4],
            .nl_ddb_va=device.ddb[3],.irp_size=PORT_V37_IRP_SIZE,
            .ucb_size=PORT_V37_UCB_SIZE,.softint=interrupt,.softint_arg=&events};
        CHECK(port_v37_nl_ddt_dispatch(ddt,sizeof ddt,DDT,RET,READ,WRITE,
            funcs[i],&req,port_v37_nl_fdt_read,
            port_v37_nl_fdt_write)==PORT_SYS85_NORMAL);
    }
    CHECK(events.count==2u&&events.level==PORT_V37_IOPOST_IPL&&q.queued==2u);
    CHECK(port_v37_iopost_validate(&q,BEGIN,END,ACTION)==PORT_V37_IOPOST_OK);
    CHECK(port_v37_iopost_validate(&q,BEGIN,END,END+3u)==PORT_V37_IOPOST_BAD);
    CHECK(port_v37_iopost_service(&q,BEGIN,END,ACTION+4u,&log)==PORT_V37_IOPOST_DEFER);
    CHECK(q.queued==2u&&log.completed==0u);
    CHECK(port_v37_iopost_drain(&q,BEGIN,END,ACTION,&log,3u)==PORT_V37_IOPOST_BAD);
    CHECK(q.queued==2u&&log.completed==0u);
    port_v37_write32(pool+PORT_V37_IRP_IOSB,0x00340000u);
    CHECK(port_v37_iopost_service(&q,BEGIN,END,ACTION,&log)==PORT_V37_IOPOST_DEFER);
    port_v37_write32(pool+PORT_V37_IRP_IOSB,0u);
    port_v37_write16(pool+PORT_V37_IRP_STS,1u);
    CHECK(port_v37_iopost_service(&q,BEGIN,END,ACTION,&log)==PORT_V37_IOPOST_DEFER);
    port_v37_write16(pool+PORT_V37_IRP_STS,0u);
    port_v37_write32(pool+PORT_V37_IRP_AST,0x00340020u);
    CHECK(port_v37_iopost_service(&q,BEGIN,END,ACTION,&log)==PORT_V37_IOPOST_DEFER);
    port_v37_write32(pool+PORT_V37_IRP_AST,0u);
    port_v37_write32(pool+PORT_V37_IRP_PID,0u);
    CHECK(port_v37_iopost_service(&q,BEGIN,END,ACTION,&log)==PORT_V37_IOPOST_DEFER);
    port_v37_write32(pool+PORT_V37_IRP_PID,ACTION);
    uint32_t second_back=port_v37_read32(pool+PORT_V37_IRP_SIZE+4u);
    port_v37_write32(pool+PORT_V37_IRP_SIZE+4u,PKTS+8u);
    CHECK(port_v37_iopost_service(&q,BEGIN,END,ACTION,&log)==PORT_V37_IOPOST_CORRUPT);
    port_v37_write32(pool+PORT_V37_IRP_SIZE+4u,second_back);
    CHECK(port_v37_iopost_drain(&q,BEGIN,END,ACTION,&log,2u)==PORT_V37_IOPOST_OK);
    CHECK(log.completed==2u&&log.irp_va[0]==PKTS&&log.irp_va[1]==PKTS+PORT_V37_IRP_SIZE);
    CHECK(log.status[0]==PORT_SYS85_ENDOFFILE&&log.status[1]==PORT_SYS85_NORMAL);
    CHECK(log.secondary[0]==0u&&log.secondary[1]==0u);
    CHECK(q.queued==0u&&q.first==HEAD&&q.last==HEAD);
    CHECK(port_v37_read32(pool)==0u&&port_v37_read32(pool+4u)==0u);
    CHECK(port_v37_read32(pool+PORT_V37_IRP_SIZE)==0u&&
          port_v37_read32(pool+PORT_V37_IRP_SIZE+4u)==0u);
    CHECK(port_v37_post_validate(&q)==PORT_V37_QIO_OK);
    uint32_t ignored=0;
    CHECK(port_v37_iopost_pop(&q,&ignored)==PORT_V37_IOPOST_EMPTY);
    CHECK(port_v37_iopost_service(&q,BEGIN,END,ACTION,&log)==PORT_V37_IOPOST_FULL);
    CHECK(port_v37_read32(pool+PORT_V37_IRP_MEDIA)==PORT_SYS85_ENDOFFILE);
    CHECK(port_v37_read32(pool+PORT_V37_IRP_SIZE+PORT_V37_IRP_MEDIA)==PORT_SYS85_NORMAL);
    CHECK(port_v37_read32(db+PORT_V37_UCB_NL_OFF+PORT_V37_UCB_OPCNT)==2u);
    puts("IOCIOPOST.MAR original kernel end-action REMQUE and bounded resident IRP completion PASS");
    return 0;
}
