#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include "vms_dqddt.h"
#include "vms_dqddt_ioc_bind.h"
#include "vms_sysassign.h"
#include "vms_exe_io.h"
#include "vms_ini_devicedb.h"
#include "vms_ioc_phys.h"
#include "vms_sysqio_args.h"
#include "vms_iopost_end.h"
#define CHECK(x) do {if(!(x)){fprintf(stderr,"DQ DDT failed line %u: %s\n",(unsigned)__LINE__,#x);return 1;}}while(0)
static int mock_read(void *ctx,uint32_t lbn,uint16_t dst[256]){
    unsigned *calls=ctx;(*calls)++;
    for(unsigned i=0;i<256u;i++)dst[i]=(uint16_t)(lbn+i);
    return 0;
}
static int mock_write(void *ctx,uint32_t lbn,const uint16_t src[256]){
    unsigned *calls=ctx;(*calls)++;
    return lbn==7u&&src[0]==0x55aau&&src[255]==0xaa55u?0:-1;
}
static void mock_irq(void){}
static uint32_t assign_lock(void *v){(void)v;return 1u;}
static void assign_unlock(void *v){(void)v;}
static uint32_t assign_related(void *v,uint32_t owner,uint32_t pid){
    (void)v;return owner==pid?1u:0u;
}
static uint32_t assign_access(void *v,uint32_t arb,const uint8_t *ucb,size_t n){
    (void)v;(void)ucb;(void)n;return arb?1u:0u;
}
struct sysqio_fixture {
    struct port_v37_sysassign_qio_context channels;
    unsigned errors;
};
struct assign_user_memory {
    uint32_t pde[1024],pte[1024];
    _Alignas(4096) uint8_t bytes[4096];
};
static const uint32_t *assign_table(void *v,uint32_t address){
    struct assign_user_memory *m=v;return address==0x5000u?m->pte:0;
}
static uint8_t *assign_page(void *v,uint32_t address){
    struct assign_user_memory *m=v;return address==0x9000u?m->bytes:0;
}
static void sysqio_post_error(void *v,uint32_t pid,uint8_t efn,uint32_t status){
    struct sysqio_fixture *f=v;(void)pid;(void)efn;(void)status;f->errors++;
}
struct qio_sync_state { uint32_t ipl; };
static uint32_t qio_raise(void *v){
    struct qio_sync_state *s=v;uint32_t old=s->ipl;s->ipl=4u;return old;
}
static void qio_restore(void *v,uint32_t old){
    ((struct qio_sync_state *)v)->ipl=old;
}
int main(void){
    enum {BASE=0x00370000u,START=0x00324120u,RET=0x00324104u};
    const uint32_t action=(uint32_t)(uintptr_t)port_dqddt_bootread;
    const uint32_t write_action=(uint32_t)(uintptr_t)port_dqddt_bootwrite;
    struct port_dq_fdt fdt={0};
    CHECK(port_dq_fdt_build(&fdt,sizeof fdt)==PORT_DQ_FDT_OK);
    struct port_dqddt_stage d={0};
    CHECK(port_dqddt_stage_init(&d,&fdt,BASE,START,RET,action,write_action)==PORT_DQDDT_OK);
    CHECK(d.table_base==BASE&&d.native_start_pc==START&&d.ioc_return_pc==RET);
    CHECK(port_dqddt_validate(&d,&fdt)==PORT_DQDDT_OK);
    struct port_dqddt_native native={0};
    const uint32_t native_base=(uint32_t)(uintptr_t)native.image;
    CHECK(port_dqddt_activate(&d,&fdt,&native,native_base)==PORT_DQDDT_OK);
    CHECK(port_dqddt_native_validate(&d,&fdt,&native)==PORT_DQDDT_OK);
    CHECK(port_v37_read32(native.image)==START&&
        port_v37_read32(native.image+8u)==native_base+PORT_DQDDT_HEADER_BYTES);
    CHECK(port_v37_read32(d.image)==(uint32_t)(START-BASE));
    CHECK(port_v37_read32(d.image+4u)==RET);
    CHECK(port_v37_read32(d.image+8u)==PORT_DQDDT_HEADER_BYTES);
    CHECK(port_v37_read16(d.image+20u)==84u);
    CHECK(port_v37_read16(d.image+36u)==136u);
    CHECK(port_dqddt_entry_offset(0u)==40u&&port_dqddt_entry_offset(1u)==48u);
    CHECK(port_dqddt_entry_offset(2u)==56u&&port_dqddt_entry_offset(11u)==164u);
    CHECK(port_dqddt_entry_offset(12u)==UINT32_MAX);
    CHECK(port_v37_read32(d.image+port_dqddt_entry_offset(2u))==(uint32_t)fdt.mask[2u]);
    CHECK(port_v37_read32(d.image+port_dqddt_entry_offset(2u)+4u)==(uint32_t)(fdt.mask[2u]>>32u));
    CHECK(port_v37_read32(d.image+port_dqddt_entry_offset(2u)+8u)==
          (action|UINT32_C(0x80000000)));
    CHECK(port_v37_read32(d.image+port_dqddt_entry_offset(PORT_DQ_FDT_WRITE)+8u)==
          (write_action|UINT32_C(0x80000000)));
    CHECK(port_dqddt_function(&d,&fdt,PORT_DQ_READPBLK)==PORT_DQDDT_NATIVE);
    CHECK(port_dqddt_function(&d,&fdt,PORT_DQ_WRITEPBLK)==PORT_DQDDT_NATIVE);
    CHECK(port_dqddt_function(&d,&fdt,
        (uint16_t)(PORT_DQ_READPBLK|0x0100u))==PORT_DQDDT_NATIVE);
    CHECK(port_dqddt_function(&d,&fdt,
        (uint16_t)(PORT_DQ_WRITEPBLK|0x8000u))==PORT_DQDDT_NATIVE);
    CHECK(port_dqddt_function(&d,&fdt,PORT_DQ_WRITELBLK)==PORT_DQDDT_UNBOUND);
    CHECK(port_dqddt_function(&d,&fdt,PORT_DQ_READVBLK)==PORT_DQDDT_UNBOUND);
    CHECK(port_dqddt_function(&d,&fdt,63u)==PORT_DQDDT_BAD);
    CHECK(port_dqddt_function(&d,&fdt,UINT16_MAX)==PORT_DQDDT_BAD);
    for(unsigned i=0;i<PORT_DQ_FDT_ROWS;i++){
        uint32_t pos=port_dqddt_entry_offset(i);
        uint8_t old=d.image[pos];d.image[pos]^=1u;
        CHECK(port_dqddt_validate(&d,&fdt)==PORT_DQDDT_BAD);
        d.image[pos]=old;
        if(i>=2u){old=d.image[pos+8u];d.image[pos+8u]=1u;
            CHECK(port_dqddt_validate(&d,&fdt)==PORT_DQDDT_BAD);
            d.image[pos+8u]=old;}
    }
    CHECK(port_dqddt_validate(&d,&fdt)==PORT_DQDDT_OK);
    d.image[20u]=0u;
    CHECK(port_dqddt_validate(&d,&fdt)==PORT_DQDDT_BAD);
    d.image[20u]=PORT_DQDDT_SOURCE_DIAG_BYTES;
    d.image[32u]=1u;
    CHECK(port_dqddt_validate(&d,&fdt)==PORT_DQDDT_BAD);
    d.image[32u]=0u;
    d.driver_bound=1u;
    CHECK(port_dqddt_validate(&d,&fdt)==PORT_DQDDT_BAD);
    d.driver_bound=0u;
    CHECK(port_dqddt_validate(&d,&fdt)==PORT_DQDDT_OK);

    /* Exercise both staged private physical action slots. */
    uint32_t db_words[PORT_V37_DEVICE_BYTES/4u]={0};
    uint32_t irp_words[PORT_V37_IRP_SIZE/4u]={0};
    uint16_t sector[256]={0};
    struct port_v37_device_result info={0};
    uint8_t *db=(uint8_t *)db_words,*irp=(uint8_t *)irp_words;
    CHECK(port_v37_devices_prelink(db,sizeof db_words,
        (uint32_t)(uintptr_t)db,&info)==PORT_V37_DEV_OK);
    port_v37_write32(db+PORT_V37_DDB_NL_OFF+12u,0x00700000u);
    port_v37_write32(db+PORT_V37_UCB_NL_OFF+120u,0x00700000u);
    info.bound_nl_ddt=0x00700000u;info.incomplete_driver_mask=7u;
    db[20u]=3u;db[21u]='D';db[22u]='Q';db[23u]='A';db[36u]=8u;
    {const char name[]="DQDRIVER";for(unsigned i=0;i<8u;i++)db[37u+i]=(uint8_t)name[i];}
    info.boot_name_ready=1u;
    CHECK(port_v37_devices_check(db,sizeof db_words,
        (uint32_t)(uintptr_t)db,&info)==PORT_V37_DEV_OK);
    CHECK(port_v37_irp_prepare(irp,PORT_V37_IRP_SIZE,0x00712000u,
        info.ucb[0],PORT_DQ_READPBLK)==PORT_V37_QIO_OK);
    port_v37_write16(irp+42u,PORT_DQ_PHYSIO);
    port_v37_write16(irp+50u,PORT_DQ_BOOTREAD_BYTES);
    port_v37_write32(irp+PORT_V37_IRP_MEDIA,7u);
    unsigned calls=0u;
    struct port_dq_bootread_request request={.fdt=&fdt,.device_db=db,
        .device_info=&info,.irp=irp,.irp_bytes=PORT_V37_IRP_SIZE,
        .sector=sector,.sector_bytes=sizeof sector,.boot_unit=0u,
        .read_sector=mock_read,.write_sector=mock_write,.read_context=&calls};
    struct port_dq_bootread_result read_result={0};
    CHECK(port_dqddt_startio(&d,&native,&fdt,&request,&read_result)==
        PORT_DQ_BOOTREAD_OK);
    CHECK(calls==1u&&read_result.lbn==7u&&read_result.bytes==512u&&
        read_result.irp_status==1u&&sector[0]==7u&&sector[255]==262u);
    port_v37_write16(irp+PORT_V37_IRP_FUNC,PORT_DQ_READVBLK);
    CHECK(port_dqddt_startio(&d,&native,&fdt,&request,&read_result)==
        PORT_DQ_BOOTREAD_UNBOUND);
    CHECK(calls==1u);
    port_v37_write16(irp+PORT_V37_IRP_FUNC,63u);
    CHECK(port_dqddt_startio(&d,&native,&fdt,&request,&read_result)==
        PORT_DQ_BOOTREAD_BAD);
    CHECK(calls==1u);
    port_v37_write16(irp+PORT_V37_IRP_FUNC,PORT_DQ_READPBLK);
    CHECK(port_dqddt_dispatch_bootread(&d,&fdt,&request,&read_result,8u)==
        PORT_DQ_BOOTREAD_BAD);
    CHECK(calls==1u);
    memset(irp,0,PORT_V37_IRP_SIZE);
    CHECK(port_v37_irp_prepare(irp,PORT_V37_IRP_SIZE,0x00712000u,
        info.ucb[0],PORT_DQ_WRITEPBLK)==PORT_V37_QIO_OK);
    port_v37_write16(irp+42u,PORT_DQ_PHYSIO);
    port_v37_write16(irp+50u,PORT_DQ_BOOTREAD_BYTES);
    port_v37_write32(irp+PORT_V37_IRP_MEDIA,7u);
    sector[0]=0x55aau;sector[255]=0xaa55u;
    uint32_t write_status=port_dqddt_startio(&d,&native,&fdt,&request,
        &read_result);
    CHECK(write_status==PORT_DQ_BOOTREAD_OK);
    CHECK(calls==2u&&read_result.io_function==PORT_DQ_WRITEPBLK&&
        read_result.irp_status==1u&&read_result.bytes==512u);
    struct port_dq_crb crb={0};
    CHECK(port_dq_crb_init(&crb,mock_irq,PORT_DQ_CRB_IRQ14_VECTOR)==
        PORT_DQ_CRB_OK);
    info.bound_dqa_crb=(uint32_t)(uintptr_t)&crb;
    port_v37_write32(db+PORT_V37_UCB_BOOT_OFF+32u,info.bound_dqa_crb);
    CHECK(port_v37_devices_check(db,sizeof db_words,
        (uint32_t)(uintptr_t)db,&info)==PORT_V37_DEV_OK);
    CHECK(port_dqddt_publish_ioc(db,sizeof db_words,
        (uint32_t)(uintptr_t)db,&info,&d,&fdt,&native,&crb)==
            PORT_DQDDT_BIND_OK);
    CHECK(info.incomplete_driver_mask==6u&&
        info.bound_dqa_ddt==native_base&&
        port_v37_read32(db+PORT_V37_DDB_BOOT_OFF+12u)==native_base&&
        port_v37_read32(db+PORT_V37_UCB_BOOT_OFF+120u)==native_base);
    CHECK(port_v37_devices_check(db,sizeof db_words,
        (uint32_t)(uintptr_t)db,&info)==PORT_V37_DEV_OK);
    const uint32_t bound_base=port_v37_read32(db+PORT_V37_DDB_BOOT_OFF+12u);
    CHECK(port_dqddt_publish_ioc(db,sizeof db_words,
        (uint32_t)(uintptr_t)db,&info,&d,&fdt,&native,&crb)==
            PORT_DQDDT_BIND_BAD);
    CHECK(port_v37_read32(db+PORT_V37_DDB_BOOT_OFF+12u)==bound_base);
    struct port_v37_ioc_result found={0};
    CHECK(port_v37_ioc_searchdev(db,sizeof db_words,
        (uint32_t)(uintptr_t)db,&info,"DQA0",4u,&found)==PORT_V37_IOC_OK);
    CHECK(found.ddb==info.ddb[0]&&found.ucb==info.ucb[0]&&
        found.unit==0u&&found.driver_bound);
    uint8_t ccb_storage[64]={0};uint8_t *ccb_base=ccb_storage+48u;
    uint16_t high_water=0u,assigned=UINT16_MAX;
    const struct port_v37_assign_ops assign_ops={0,assign_lock,assign_unlock,
        assign_related,assign_access};
    const struct port_v37_sysassign_local_request assign_request={
        .device_database=db,.device_database_bytes=sizeof db_words,
        .device_database_base=(uint32_t)(uintptr_t)db,.devices=&info,
        .device_name="DQA0",.device_name_bytes=4u,.ccb_base=ccb_base,
        .ccb_preceding_bytes=48u,.channel_count=3u,
        .high_water_channel=&high_water,.process_pid=0x12340001u,
        .arb_va=0x0034c000u,.effective_max_acmode=0u,
        .assign_ops=&assign_ops};
    struct assign_user_memory assign_memory={0};
    assign_memory.pde[0]=0x5007u;assign_memory.pte[1]=0x9007u;
    assign_memory.bytes[0]=4u;assign_memory.bytes[1]=0u;
    assign_memory.bytes[2]=14u;assign_memory.bytes[3]=1u;
    assign_memory.bytes[4]=0x10u;assign_memory.bytes[5]=0x10u;
    memcpy(assign_memory.bytes+16u,"DQA0",4u);
    const struct port_user_map assign_map={assign_memory.pde,
        assign_table,&assign_memory};
    struct port_exe_access assign_access;
    struct port_exe_user_access assign_binding;
    port_exe_user_access_init(&assign_access,&assign_binding,&assign_map,
        assign_page,&assign_memory);
    const struct port_v37_sysassign_descriptor_request assign_descriptor={
        &assign_request,&assign_access,0x1000u,2u,0u};
    CHECK(port_v37_sysassign_descriptor_service(&assign_descriptor,0x1040u)==
        PORT_V37_CCB_OK);
    assigned=port_v37_read16(assign_memory.bytes+64u);
    CHECK(assigned==16u&&high_water==16u&&
        port_v37_read32(ccb_base-16u+PORT_V37_CCB_UCB)==info.ucb[0]&&
        (ccb_base-16u)[PORT_V37_CCB_AMOD]==3u);
    memcpy(assign_memory.bytes+16u,"DQB0",4u);
    assign_memory.bytes[64]=0xffu;assign_memory.bytes[65]=0xffu;
    CHECK(port_v37_sysassign_descriptor_service(&assign_descriptor,0x1040u)==
        PORT_SS_NOSUCHDEV&&port_v37_read16(assign_memory.bytes+64u)==0u);
    memcpy(assign_memory.bytes+16u,"DQ?0",4u);
    CHECK(port_v37_sysassign_descriptor_service(&assign_descriptor,0x1040u)==
        PORT_SS_IVDEVNAM&&port_v37_read16(assign_memory.bytes+64u)==0u);
    memcpy(assign_memory.bytes+16u,"DQA0",4u);
    struct port_v37_ccb_view assigned_view={0};
    CHECK(port_v37_ccb_resolve(ccb_base,48u,assigned,48u,0u,
        &assigned_view)==PORT_V37_CCB_OK&&assigned_view.ucb==info.ucb[0]);
    struct port_v37_sysqio_device qio_device={
        .fdt=&fdt,.fdt_bytes=sizeof fdt,.online=1u};
    struct sysqio_fixture qio_fixture={.channels={
        .ccb_base=ccb_base,.ccb_preceding_bytes=48u,
        .maximum_channel_offset=48u,.owner_pid=0x12340001u,
        .device_database=db,.device_database_bytes=sizeof db_words,
        .device_database_base=(uint32_t)(uintptr_t)db,
        .assigned_ucb=info.ucb[0],.device=&qio_device}};
    uint32_t sysqio_ap[13]={12u,1u,assigned,PORT_DQ_READPBLK,0u};
    struct port_v37_sysqio_process qio_process={
        .pid=0x12340001u,.access_mode=0u,.direct_io_count=1};
    const struct port_v37_sysqio_begin_ops qio_ops={
        .context=&qio_fixture,.ccb_base=ccb_base,
        .ccb_preceding_bytes=48u,.ccb_maximum_channel_offset=48u,
        .lookup_channel=port_v37_sysassign_lookup_qio_channel,
        .post_event=sysqio_post_error};
    struct port_v37_sysqio_prepared qio_prepared={0};
    uint32_t qio_status=0u;
    CHECK(port_v37_sysqio_begin(sysqio_ap,sizeof sysqio_ap,&qio_process,
        &qio_ops,&qio_prepared,&qio_status)==PORT_V37_SYSQIO_BEGIN_OK);
    CHECK(qio_status==PORT_V37_SYSQIO_BEGIN_OK&&
        qio_prepared.channel.ucb==(void *)(db+(info.ucb[0]-
            (uint32_t)(uintptr_t)db))&&
        qio_prepared.dispatch.function_code==PORT_DQ_READPBLK&&
        qio_process.direct_io_count==0&&qio_fixture.errors==0u);
    CHECK(port_v37_sysassign_lookup_qio_channel(&qio_fixture.channels,
        0x12340002u,assigned,&qio_prepared.channel)!=0);
    uint16_t bad_channel=UINT16_MAX;
    struct port_v37_sysassign_descriptor_request bad_descriptor=
        assign_descriptor;
    bad_descriptor.device_name_descriptor=0x2000u;
    assign_memory.bytes[64]=0xffu;assign_memory.bytes[65]=0xffu;
    CHECK(port_v37_sysassign_descriptor_service(&bad_descriptor,0x1040u)==
        PORT_SYS85_ACCVIO&&port_v37_read16(assign_memory.bytes+64u)==0u);
    bad_descriptor=assign_descriptor;bad_channel=UINT16_MAX;
    assign_memory.bytes[0]=64u;
    CHECK(port_v37_sysassign_descriptor(&bad_descriptor,&bad_channel)==
        PORT_SS_IVLOGNAM&&bad_channel==0u);
    assign_memory.bytes[0]=4u;assign_memory.bytes[4]=0u;
    assign_memory.bytes[5]=0u;assign_memory.bytes[6]=0u;
    assign_memory.bytes[7]=0u;bad_channel=UINT16_MAX;
    CHECK(port_v37_sysassign_descriptor(&assign_descriptor,&bad_channel)==
        PORT_SS_ACCVIO&&bad_channel==0u);
    assign_memory.bytes[4]=0x10u;assign_memory.bytes[5]=0x10u;
    assign_memory.bytes[0]=4u;
    /* Exercise the complete host-side route through standard IRP reservation
     * using the channel created above. Hardware submission remains separate. */
    uint8_t qio_pool_memory[512u]={0};
    const struct port_ini_packet_area qio_pool_area={
        .memory=qio_pool_memory,.address=0x00380000u,
        .available_bytes=sizeof qio_pool_memory,.count=1u,
        .packet_bytes=144u,.minimum_bytes=144u,
        .sentinel_address=0x00381000u};
    struct port_ini_packet_list qio_pool={0};
    CHECK(port_ini_packet_init(&qio_pool_area,&qio_pool)==PORT_INI_PACKETS_OK);
    struct qio_sync_state qio_sync_state={.ipl=1u};
    const struct port_v37_irp_pool_sync qio_sync={
        &qio_sync_state,qio_raise,qio_restore};
    const struct port_v37_sysqio_irp_spec qio_spec={
        .ucb_va=info.ucb[0],.arb_va=0x0034c000u,
        .channel_index=assigned,.base_priority=8u};
    uint8_t *qio_irp=0;uint32_t qio_irp_va=0u;
    qio_process.direct_io_count=1;
    qio_process.has_phy_io=1u;
    CHECK(port_v37_sysqio_prepare_irp(sysqio_ap,sizeof sysqio_ap,
        &qio_process,&qio_ops,&qio_spec,&qio_pool,&qio_sync,&qio_prepared,
        &qio_irp,&qio_irp_va,&qio_status)==PORT_V37_SYSQIO_PREPARE_OK);
    CHECK(qio_process.direct_io_count==0&&qio_fixture.errors==0u&&
        qio_irp==qio_pool_memory&&qio_irp_va==0x00380000u&&
        port_v37_read32(qio_irp+12u)==qio_process.pid&&
        port_v37_read32(qio_irp+28u)==info.ucb[0]&&
        port_v37_read16(qio_irp+32u)==PORT_DQ_READPBLK&&
        port_v37_read16(qio_irp+40u)==(uint16_t)(0u-assigned)&&
        (port_v37_read16(qio_irp+42u)&0x0100u)!=0u);
    uint16_t channel_ioc=0u;uint32_t deaccess_irp=UINT32_MAX;
    CHECK(port_v37_sysassign_qio_charge(&qio_fixture.channels,
        qio_process.pid,assigned,qio_process.access_mode,&assign_ops,
        &channel_ioc)==PORT_V37_CCB_OK&&
        channel_ioc==1u&&port_v37_read16(ccb_base-16u+
        PORT_V37_CCB_IOC)==1u);
    CHECK(port_v37_iopost_ccb_complete(ccb_base,48u,
        port_v37_read16(qio_irp+40u),0,&channel_ioc,&deaccess_irp)==
        PORT_V37_IOPOST_OK&&channel_ioc==0u&&deaccess_irp==0u&&
        port_v37_read16(ccb_base-16u+PORT_V37_CCB_IOC)==0u);
    channel_ioc=0xaaaau;
    CHECK(port_v37_sysassign_qio_charge(&qio_fixture.channels,
        qio_process.pid,assigned,3u,&assign_ops,&channel_ioc)==
        PORT_V37_CCB_NOPRIV&&channel_ioc==0xaaaau&&
        port_v37_read16(ccb_base-16u+PORT_V37_CCB_IOC)==0u);
    CHECK(port_v37_irp_pool_free(&qio_pool,qio_irp,qio_irp_va,
        PORT_V37_SYSQIO_IRP_STANDARD_BYTES,&qio_sync)==PORT_V37_IRP_POOL_OK);
    assigned=UINT16_MAX;
    struct port_v37_sysassign_local_request missing_assign=assign_request;
    missing_assign.device_name="ZZZ0";
    CHECK(port_v37_sysassign_local(&missing_assign,&assigned)==
        PORT_V37_IOC_NO_SUCH&&assigned==0u);
    port_v37_write16(irp+PORT_V37_IRP_FUNC,PORT_DQ_READPBLK);
    port_v37_write32(irp+PORT_V37_IRP_MEDIA,9u);
    port_v37_write32(irp+PORT_V37_IRP_IOST2,0u);
    CHECK(port_dq_bootread_preflight(&request)==PORT_DQ_BOOTREAD_BAD);
    struct port_dq_bootread_request published_request=request;
    published_request.active_ddt_base=native_base;
    CHECK(port_dq_bootread_preflight(&published_request)==PORT_DQ_BOOTREAD_OK);
    const uint32_t ucb_crb=port_v37_read32(db+PORT_V37_UCB_BOOT_OFF+32u);
    port_v37_write32(db+PORT_V37_UCB_BOOT_OFF+32u,ucb_crb+4u);
    CHECK(port_dq_bootread_preflight(&published_request)==PORT_DQ_BOOTREAD_BAD);
    port_v37_write32(db+PORT_V37_UCB_BOOT_OFF+32u,ucb_crb);
    port_v37_write32(db+PORT_V37_UCB_BOOT_OFF+120u,native_base+4u);
    CHECK(port_dq_bootread_preflight(&published_request)==PORT_DQ_BOOTREAD_BAD);
    port_v37_write32(db+PORT_V37_UCB_BOOT_OFF+120u,native_base);
    port_v37_write32(db+PORT_V37_DDB_BOOT_OFF+12u,native_base+4u);
    CHECK(port_dq_bootread_preflight(&published_request)==PORT_DQ_BOOTREAD_BAD);
    port_v37_write32(db+PORT_V37_DDB_BOOT_OFF+12u,native_base);
    uint32_t published_lbn=0u;uint16_t published_function=0u;
    CHECK(port_dq_bootread_decode_physical(&published_request,&published_lbn,
        &published_function,9u)==PORT_DQ_BOOTREAD_OK);
    CHECK(port_dqddt_startio(&d,&native,&fdt,&request,&read_result)==
        PORT_DQ_BOOTREAD_OK);
    CHECK(calls==3u&&read_result.lbn==9u&&sector[0]==9u&&sector[255]==264u);
    struct port_dqddt_stage before=d;
    CHECK(port_dqddt_stage_init(&d,&fdt,BASE+1u,START,RET,action,write_action)==PORT_DQDDT_BAD);
    CHECK(!memcmp(&before,&d,sizeof d));
    CHECK(port_dqddt_stage_init(&d,&fdt,BASE,BASE+4u,RET,action,write_action)==PORT_DQDDT_BAD);
    CHECK(!memcmp(&before,&d,sizeof d));
    CHECK(port_dqddt_stage_init(&d,&fdt,BASE,START,BASE+8u,action,write_action)==PORT_DQDDT_BAD);
    CHECK(!memcmp(&before,&d,sizeof d));
    puts("DQ native DDT activation, translated SYSASSIGN descriptor/channel output, DQA0 IOC/CCB, SYSQIO IRP/CCB accounting and privileged READPBLK/WRITEPBLK dispatch PASS; public QIO/ACP remain unimplemented");
    return 0;
}
