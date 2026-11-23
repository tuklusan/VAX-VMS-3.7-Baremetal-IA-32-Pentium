#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include "vms_nl_ddt.h"
#define REQUIRE(x) do{if(!(x)){fprintf(stderr,"NLA DDT FAIL line %d: %s\n",__LINE__,#x);return 1;}}while(0)
struct interrupt_log {unsigned count;uint8_t level;};
static void interrupt(void *a,uint8_t level){
    struct interrupt_log *log=a;log->count++;log->level=level;
}
int main(void){
    enum {DB=0x00342000u,DDT=0x00345000u,RET=0x00341101u,
          READ=0x00341117u,WRITE=0x00341129u,PACKETS=0x00346000u,
          HEAD=0x00346200u};
    uint32_t dbwords[PORT_V37_DEVICE_BYTES/4]={0};
    uint8_t *db=(uint8_t*)dbwords;
    struct port_v37_device_result d={0};
    REQUIRE(port_v37_devices_prelink(db,sizeof dbwords,DB,&d)==PORT_V37_DEV_OK);
    REQUIRE(d.incomplete_driver_mask==15u&&d.bound_nl_ddt==0);
    uint8_t table[PORT_V37_NL_IMAGE_BYTES];
    memset(table,0xaa,sizeof table);
    REQUIRE(port_v37_nl_ddt_build(0,sizeof table,DDT,RET,READ,WRITE)==PORT_V37_NL_DDT_BAD);
    REQUIRE(port_v37_nl_ddt_build(table,sizeof table-1u,DDT,RET,READ,WRITE)==PORT_V37_NL_DDT_BAD);
    REQUIRE(table[0]==0xaa);
    REQUIRE(port_v37_nl_ddt_build(table,sizeof table,DDT,RET,READ,WRITE)==PORT_V37_NL_DDT_OK);
    REQUIRE(port_v37_nl_ddt_check(table,sizeof table,DDT,RET,READ,WRITE)==PORT_V37_NL_DDT_OK);
    REQUIRE(port_v37_read32(table+0u)==RET && port_v37_read32(table+8u)==40u);
    REQUIRE(port_v37_read16(table+36u)==40u&&port_v37_read32(table+32u)==0u);
    REQUIRE(port_v37_read32(table+44u)==0x30103u);
    REQUIRE(port_v37_read32(table+60u)==0x20002u);
    REQUIRE(port_v37_read32(table+72u)==0x10101u);
    uint8_t original=(uint8_t)table[63u];table[63u]^=1u;
    REQUIRE(port_v37_nl_ddt_check(table,sizeof table,DDT,RET,READ,WRITE)==PORT_V37_NL_DDT_BAD);
    table[63u]=original;
    REQUIRE(port_v37_nl_ddt_bind(db,sizeof dbwords,DB,&d,0)==PORT_V37_NL_DDT_BAD);
    REQUIRE(port_v37_devices_check(db,sizeof dbwords,DB,&d)==PORT_V37_DEV_OK);
    struct port_v37_ioc_result found={0};
    REQUIRE(port_v37_ioc_searchdev(db,sizeof dbwords,DB,&d,"NLA0:",5,&found)==PORT_V37_IOC_UNBOUND);
    REQUIRE(port_v37_nl_ddt_bind(db,sizeof dbwords,DB,&d,DDT)==PORT_V37_NL_DDT_OK);
    REQUIRE(d.incomplete_driver_mask==7u&&d.bound_nl_ddt==DDT);
    REQUIRE(port_v37_devices_check(db,sizeof dbwords,DB,&d)==PORT_V37_DEV_OK);
    REQUIRE(port_v37_read32(db+PORT_V37_DDB_NL_OFF+12u)==DDT);
    REQUIRE(port_v37_read32(db+PORT_V37_UCB_NL_OFF+120u)==DDT);
    REQUIRE(port_v37_nl_ddt_bind(db,sizeof dbwords,DB,&d,DDT)==PORT_V37_NL_DDT_BAD);
    REQUIRE(port_v37_nl_ddt_source_lookup(db,sizeof dbwords,DB,&d,"NLA0:",5,&found)==PORT_V37_NL_DDT_OK);
    REQUIRE(found.driver_bound&&found.ddb==d.ddb[3]&&found.ucb==d.ucb[4]);
    REQUIRE(port_v37_ioc_searchdev(db,sizeof dbwords,DB,&d,"OPA0:",5,&found)==PORT_V37_IOC_UNBOUND);
    uint32_t packets[3][PORT_V37_IRP_POOL_STRIDE/4]={0};
    struct port_v37_iopost_queue q={0};
    struct interrupt_log irq={0};
    REQUIRE(port_v37_post_init(&q,(uint8_t*)packets,sizeof packets,PACKETS,PORT_V37_IRP_POOL_STRIDE,HEAD)==PORT_V37_QIO_OK);
    const uint16_t fn[3]={PORT_SYS85_READVBLK,PORT_SYS85_WRITELBLK,PORT_SYS85_WRITEOF};
    const uint32_t statuses[3]={PORT_SYS85_ENDOFFILE,PORT_SYS85_NORMAL,PORT_SYS85_NORMAL};
    for(unsigned i=0;i<3u;i++){
        uint8_t *packet=(uint8_t*)packets[i];
        const uint32_t packet_va=PACKETS+i*PORT_V37_IRP_POOL_STRIDE;
        REQUIRE(port_v37_irp_prepare(packet,PORT_V37_IRP_SIZE,packet_va,d.ucb[4],fn[i])==PORT_V37_QIO_OK);
        struct port_v37_qio_request req={
           .post=&q,.irp=packet,.ucb=db+PORT_V37_UCB_NL_OFF,
           .irp_va=packet_va,.ucb_va=d.ucb[4],.nl_ddb_va=d.ddb[3],
           .irp_size=PORT_V37_IRP_SIZE,.ucb_size=PORT_V37_UCB_SIZE,
           .softint=interrupt,.softint_arg=&irq};
        REQUIRE(port_v37_nl_ddt_dispatch(table,sizeof table,DDT,RET,READ,WRITE,
           fn[i],&req,port_v37_nl_fdt_read,port_v37_nl_fdt_write)==PORT_SYS85_NORMAL);
        REQUIRE(port_v37_read32(packet+PORT_V37_IRP_MEDIA)==statuses[i]);
        REQUIRE(port_v37_read32(packet+PORT_V37_IRP_IOST2)==0u);
        REQUIRE(q.queued==i+1u && irq.count==i+1u&&irq.level==4u);
        REQUIRE(port_v37_post_validate(&q)==PORT_V37_QIO_OK);
        REQUIRE(port_v37_nl_ddt_dispatch(table,sizeof table,DDT,RET,READ,WRITE,
           fn[i],&req,port_v37_nl_fdt_read,port_v37_nl_fdt_write)==PORT_V37_QIO_DUP);
    }
    REQUIRE(port_v37_read32(db+PORT_V37_UCB_NL_OFF+96u)==3u);
    REQUIRE(port_v37_read32((uint8_t*)packets[0])==PACKETS+PORT_V37_IRP_POOL_STRIDE);
    REQUIRE(port_v37_read32((uint8_t*)packets[2])==HEAD);
    REQUIRE(port_v37_read32((uint8_t*)packets[2]+4u)==PACKETS+PORT_V37_IRP_POOL_STRIDE);
    uint8_t bad=(uint8_t)table[72u];table[72u]^=2u;
    REQUIRE(port_v37_nl_ddt_check(table,sizeof table,DDT,RET,READ,WRITE)==PORT_V37_NL_DDT_BAD);
    table[72u]=bad;
    REQUIRE(port_v37_post_validate(&q)==PORT_V37_QIO_OK);
    REQUIRE(port_v37_read32(db+PORT_V37_DDB_NL_OFF+12u)==DDT);
    puts("NLDRIVER.MAR source DDTAB/FUNCTAB native IOC NLA DDT, read/write and queued IRPs PASS");
    return 0;
}
