#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include "vms_iopost_end.h"
#include "vms_nl_ddt.h"
#define CHECK(x) do { if(!(x)) {fprintf(stderr,"IOPOST end FAIL %d: %s\n",__LINE__,#x);return 1;} }while(0)
struct irq {uint32_t count,level;};
static void interrupt(void *v,uint8_t level){struct irq *p=v;p->count++;p->level=level;}
struct acp_wake {uint32_t calls,irp,ucb;};
static void wake_acp(void *v,uint32_t irp,uint32_t ucb){
    struct acp_wake *w=v;++w->calls;w->irp=irp;w->ucb=ucb;
}
struct dirpost_rig {
    uint32_t order,pid,efn,irp_va,iosb,iosb_mode,status,secondary;
    uint8_t buffered;
};
static void dirpost_account(void *v,uint32_t pid,uint8_t buffered){
    struct dirpost_rig *r=v;r->order++;r->pid=pid;r->buffered=buffered;
}
static void dirpost_wake(void *v,uint32_t irp,uint32_t ucb){
    struct dirpost_rig *r=v;r->order++;
    if(irp!=0x5678u||ucb!=0x1234u)r->order=99u;
}
static void dirpost_iosb(void *v,uint32_t iosb,uint8_t mode,
    uint32_t status,uint32_t secondary){
    struct dirpost_rig *r=v;r->order++;r->iosb=iosb;r->iosb_mode=mode;
    r->status=status;r->secondary=secondary;
}
static void dirpost_event(void *v,uint32_t pid,uint8_t efn){
    struct dirpost_rig *r=v;r->order++;r->pid=pid;r->efn=efn;
}
static void dirpost_ast(void *v,uint32_t pid,uint32_t irp_va){
    struct dirpost_rig *r=v;r->order++;r->pid=pid;r->irp_va=irp_va;
}
static void dirpost_free(void *v,uint32_t pid,uint32_t irp_va){
    struct dirpost_rig *r=v;r->order++;r->pid=pid;r->irp_va=irp_va;
}
int main(void){
    _Alignas(4) uint8_t ccb_bytes[64]={0};
    uint8_t *const ccb_base=ccb_bytes+48u;
    uint8_t *const ccb=ccb_base-32u;
    uint16_t outstanding=UINT16_C(0xaaaa);uint32_t deaccess=UINT32_MAX;
    port_v37_write16(ccb+PORT_V37_CCB_IOC,2u);
    CHECK(port_v37_iopost_ccb_complete(ccb_base,48u,
          (uint16_t)(0u-32u),0,&outstanding,&deaccess)==PORT_V37_IOPOST_OK);
    CHECK(outstanding==1u&&deaccess==0u&&
          port_v37_read16(ccb+PORT_V37_CCB_IOC)==1u);
    port_v37_write16(ccb+PORT_V37_CCB_IOC,1u);
    port_v37_write32(ccb+PORT_V37_CCB_DIRP,0x00005678u);
    port_v37_write32(ccb+PORT_V37_CCB_UCB,0x00001234u);
    CHECK(port_v37_iopost_ccb_complete(ccb_base,48u,
          (uint16_t)(0u-32u),0,&outstanding,&deaccess)==
          PORT_V37_IOPOST_CCB_DEFER);
    CHECK(port_v37_read16(ccb+PORT_V37_CCB_IOC)==1u&&
          port_v37_read32(ccb+PORT_V37_CCB_DIRP)==0x00005678u);
    struct acp_wake wake={0};
    const struct port_v37_iopost_ccb_ops ccb_ops={&wake,wake_acp};
    CHECK(port_v37_iopost_ccb_complete(ccb_base,48u,
          (uint16_t)(0u-32u),&ccb_ops,&outstanding,&deaccess)==
          PORT_V37_IOPOST_OK);
    CHECK(outstanding==1u&&deaccess==0x00005678u&&wake.calls==1u&&
          wake.irp==0x00005678u&&wake.ucb==0x00001234u&&
          port_v37_read16(ccb+PORT_V37_CCB_IOC)==1u&&
          port_v37_read32(ccb+PORT_V37_CCB_DIRP)==0u);
    port_v37_write16(ccb+PORT_V37_CCB_IOC,0u);
    port_v37_write32(ccb+PORT_V37_CCB_DIRP,0u);
    CHECK(port_v37_iopost_ccb_complete(ccb_base,48u,
          (uint16_t)(0u-32u),&ccb_ops,&outstanding,&deaccess)==
          PORT_V37_IOPOST_OK);
    CHECK(outstanding==UINT16_MAX&&port_v37_read16(ccb+PORT_V37_CCB_IOC)==
          UINT16_MAX&&wake.calls==1u);
    port_v37_write16(ccb+PORT_V37_CCB_IOC,3u);
    CHECK(port_v37_iopost_ccb_complete(ccb_base,48u,UINT16_MAX,
          &ccb_ops,&outstanding,&deaccess)==PORT_V37_IOPOST_BAD);
    CHECK(port_v37_read16(ccb+PORT_V37_CCB_IOC)==3u);
    _Alignas(4) uint8_t dirpost_irp[PORT_V37_IRP_SIZE]={0};
    port_v37_write16(dirpost_irp+8u,PORT_V37_IRP_SIZE);
    port_v37_write32(dirpost_irp+12u,0x10007u);
    port_v37_write32(dirpost_irp+16u,0x00345678u);
    port_v37_write32(dirpost_irp+36u,0x00401000u);
    dirpost_irp[11u]=0x43u;
    dirpost_irp[34u]=7u;
    port_v37_write16(dirpost_irp+40u,(uint16_t)(0u-32u));
    port_v37_write16(dirpost_irp+42u,1u);
    port_v37_write32(dirpost_irp+56u,0x00000001u);
    port_v37_write32(dirpost_irp+60u,512u);
    port_v37_write16(ccb+PORT_V37_CCB_IOC,1u);
    port_v37_write32(ccb+PORT_V37_CCB_DIRP,0x5678u);
    port_v37_write32(ccb+PORT_V37_CCB_UCB,0x1234u);
    struct dirpost_rig dirpost={0};
    const struct port_v37_iopost_dirpost_ops dirpost_ops={
        .context=&dirpost,.account_io=dirpost_account,
        .ccb={&dirpost,dirpost_wake},.ccb_base=ccb_base,
        .ccb_preceding_bytes=48u,.store_iosb_if_writable=dirpost_iosb,
        .post_event=dirpost_event,.queue_ast=dirpost_ast,
        .free_irp=dirpost_free};
    CHECK(port_v37_iopost_dirpost(dirpost_irp,sizeof dirpost_irp,
          0x00347000u,&dirpost_ops)==PORT_V37_IOPOST_OK);
    CHECK(dirpost.order==5u&&dirpost.pid==0x10007u&&dirpost.buffered&&
          dirpost.iosb==0x00401000u&&dirpost.iosb_mode==3u&&
          dirpost.status==1u&&dirpost.secondary==512u&&dirpost.efn==7u&&
          dirpost.irp_va==0x00347000u&&
          port_v37_read16(ccb+PORT_V37_CCB_IOC)==1u&&
          port_v37_read32(ccb+PORT_V37_CCB_DIRP)==0u);
    dirpost.order=0u;dirpost.irp_va=0u;dirpost_irp[11u]=2u;
    port_v37_write16(ccb+PORT_V37_CCB_IOC,2u);
    port_v37_write32(ccb+PORT_V37_CCB_DIRP,0u);
    CHECK(port_v37_iopost_dirpost(dirpost_irp,sizeof dirpost_irp,
          0x00347000u,&dirpost_ops)==PORT_V37_IOPOST_OK);
    CHECK(dirpost.order==4u&&dirpost.irp_va==0x00347000u&&
          port_v37_read16(ccb+PORT_V37_CCB_IOC)==1u);
    dirpost.order=0u;port_v37_write16(dirpost_irp+40u,0u);
    port_v37_write16(ccb+PORT_V37_CCB_IOC,2u);
    CHECK(port_v37_iopost_dirpost(dirpost_irp,sizeof dirpost_irp,
          0x00347000u,&dirpost_ops)==PORT_V37_IOPOST_BAD);
    CHECK(dirpost.order==0u&&port_v37_read16(ccb+PORT_V37_CCB_IOC)==2u);
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
