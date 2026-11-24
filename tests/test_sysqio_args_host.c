#include "vms_sysqio_args.h"
#include "vms_qio_post.h"
#include "vms_sysqio_ef.h"
#include "vms_sysqio_iosb.h"
#include "vms_sys85.h"
#include "vms_exe_schevt.h"
#include "vms_v37_dyn_tags.h"
#include "vms_qio_irp_pool.h"
#include "vms_ini_packet_queues.h"
#include "vms_v37_pcb.h"
#include "vms_v37_phd.h"
#include <stdio.h>
#include <string.h>

static void put32(uint8_t *p,uint32_t v){
    p[0]=(uint8_t)v;p[1]=(uint8_t)(v>>8);
    p[2]=(uint8_t)(v>>16);p[3]=(uint8_t)(v>>24);
}
static void put16(uint8_t *p,uint16_t v){p[0]=(uint8_t)v;p[1]=(uint8_t)(v>>8);}
static uint16_t get16(const uint8_t *p){
    return (uint16_t)((uint16_t)p[0]|((uint16_t)p[1]<<8));
}
static uint32_t get32(const uint8_t *p){
    return (uint32_t)p[0]|((uint32_t)p[1]<<8)|
           ((uint32_t)p[2]<<16)|((uint32_t)p[3]<<24);
}
struct pool_sync_state { uint32_t ipl,enters,exits; };
static uint32_t pool_raise(void *opaque){
    struct pool_sync_state *s=opaque;uint32_t old=s->ipl;s->ipl=4u;
    ++s->enters;return old;
}
static void pool_restore(void *opaque,uint32_t old){
    struct pool_sync_state *s=opaque;s->ipl=old;++s->exits;
}
struct global_ef_test { uint8_t seen;uint32_t status; };
struct volume_access_test {
    unsigned calls;
    uint8_t kind;
    uint16_t protection;
    uint32_t owner,arb,status;
};
static uint32_t check_volume(void *opaque,uint8_t kind,uint16_t protection,
    uint32_t owner,uint32_t arb){
    struct volume_access_test *t=opaque;
    t->calls++;t->kind=kind;t->protection=protection;
    t->owner=owner;t->arb=arb;return t->status;
}
static uint32_t clear_global(void *opaque,uint8_t efn){
    struct global_ef_test *t=opaque;t->seen=efn;return t->status;
}
struct iosb_memory_test {
    uint32_t base;
    uint8_t bytes[32];
    uint32_t probes,clears;
    int deny_probe,deny_clear;
};
struct transfer_access_test {
    uint32_t reads,writes,address,bytes;
    int read_ok,write_ok;
};
static int probe_transfer_read(void *opaque,uint32_t address,uint32_t bytes){
    struct transfer_access_test *t=opaque;t->reads++;
    t->address=address;t->bytes=bytes;return t->read_ok;
}
static int probe_transfer_write(void *opaque,uint32_t address,uint32_t bytes){
    struct transfer_access_test *t=opaque;t->writes++;
    t->address=address;t->bytes=bytes;return t->write_ok;
}
static uint32_t transfer_pde[1024],transfer_pte[1024];
static const uint32_t *resolve_transfer_table(void *opaque,uint32_t physical){
    (void)opaque;return physical==0x1000u?transfer_pte:0;
}
static uint8_t *resolve_transfer_page(void *opaque,uint32_t physical){
    (void)opaque;return physical?(uint8_t *)(uintptr_t)1u:0;
}
static int probe_iosb(void *opaque,uint32_t address,size_t size){
    struct iosb_memory_test *m=opaque;m->probes++;
    return m->deny_probe||address<m->base||size>sizeof m->bytes||
        address-m->base>sizeof m->bytes-size;
}
static int clear_iosb(void *opaque,uint32_t address,size_t size){
    struct iosb_memory_test *m=opaque;m->clears++;
    if(m->deny_clear||address<m->base||size>sizeof m->bytes||
       address-m->base>sizeof m->bytes-size)return 1;
    memset(m->bytes+(address-m->base),0,size);return 0;
}
struct begin_rig {
    const struct port_v37_sysqio_device *device;
    void *ucb;
    uint32_t expected_pid,expected_channel,lookups,posts,posted_pid,posted_status;
    uint8_t posted_efn,ccb_access_limit;
    uint32_t quota_calls,waits,window;
    uint8_t quota_buffered;int deny_channel,grant_quota;
    uint32_t source_ucb;
    uint8_t *source_ccb;
};
static int lookup_channel(void *opaque,uint32_t pid,uint32_t channel,
    struct port_v37_sysqio_resolved_channel *out){
    struct begin_rig *r=opaque;r->lookups++;
    if(r->deny_channel||pid!=r->expected_pid||channel!=r->expected_channel)
        return -1;
    out->device=r->device;out->ucb=r->ucb;
    out->window=r->window;
    out->ccb_access_limit=r->ccb_access_limit?r->ccb_access_limit:4u;
    return 0;
}
static int lookup_ucb(void *opaque,uint32_t pid,uint32_t ucb,
    struct port_v37_sysqio_resolved_channel *out){
    struct begin_rig *r=opaque;r->lookups++;
    if(pid!=r->expected_pid||ucb!=r->source_ucb)return -1;
    out->device=r->device;out->ucb=r->ucb;return 0;
}
static void wait_channel(void *opaque,uint32_t pid,uint32_t channel,
    struct port_v37_sysqio_resolved_channel *resolved){
    struct begin_rig *r=opaque;
    if(pid==r->expected_pid&&channel==r->expected_channel){
        ++r->waits;r->window&=~1u;resolved->window=r->window;
    }
}
static void wait_source_channel(void *opaque,uint32_t pid,uint32_t channel,
    struct port_v37_sysqio_resolved_channel *resolved){
    struct begin_rig *r=opaque;
    if(pid==r->expected_pid&&channel==32u&&r->source_ccb){
        ++r->waits;put32(r->source_ccb+PORT_V37_CCB_WIND,0u);
        resolved->window=0u;
    }
}
static int single_quota(void *opaque,uint32_t pid,uint8_t buffered){
    struct begin_rig *r=opaque;
    if(pid!=r->expected_pid)return 0;
    r->quota_calls++;r->quota_buffered=buffered;
    return r->grant_quota;
}
static void post_event(void *opaque,uint32_t pid,uint8_t efn,uint32_t status){
    struct begin_rig *r=opaque;r->posts++;r->posted_pid=pid;
    r->posted_efn=efn;r->posted_status=status;
}

int main(void){
    _Alignas(4) uint8_t ap[PORT_V37_SYSQIO_LIST_BYTES+4u]={0};
    put32(ap,PORT_V37_SYSQIO_REQUIRED_ARGS);
    const uint32_t values[12]={3u,0x12340020u,0xffff0031u,0x1000u,
        0x2000u,0xaabbccddu,1u,2u,3u,4u,5u,6u};
    for(unsigned i=0;i<12u;i++)put32(ap+4u+4u*i,values[i]);
    struct port_v37_sysqio_args out;
    memset(&out,0xa5,sizeof out);
    if(port_v37_sysqio_decode_args(ap,sizeof ap,&out)!=PORT_V37_SYSQIO_ARGS_OK||
       out.event_flag!=3u||out.channel!=0x12340020u||out.function!=0x31u||
       out.iosb!=0x1000u||out.ast_address!=0x2000u||
       out.ast_parameter!=0xaabbccddu)
        return 1;
    for(unsigned i=0;i<6u;i++)if(out.parameter[i]!=i+1u)return 2;
    struct port_v37_sysqio_args sentinel;
    memset(&sentinel,0x5a,sizeof sentinel);out=sentinel;
    put32(ap,11u);
    if(port_v37_sysqio_decode_args(ap,sizeof ap,&out)!=PORT_V37_SYSQIO_ARGS_BAD||
       memcmp(&out,&sentinel,sizeof out))return 3;
    put32(ap,12u);
    if(port_v37_sysqio_decode_args(ap,PORT_V37_SYSQIO_LIST_BYTES-1u,&out)!=
       PORT_V37_SYSQIO_ARGS_BAD||memcmp(&out,&sentinel,sizeof out))return 4;
    if(port_v37_sysqio_decode_args(ap+1u,sizeof ap-1u,&out)!=
       PORT_V37_SYSQIO_ARGS_BAD)return 5;
    struct port_dq_fdt fdt;
    if(port_dq_fdt_build(&fdt,sizeof fdt)!=PORT_DQ_FDT_OK)return 6;
    struct port_v37_sysqio_device device={.fdt=&fdt,
        .fdt_bytes=sizeof fdt,.online=1u};
    uint8_t access_limit=4u;
    struct port_v37_sysqio_dispatch action={0};
    struct port_v37_sysqio_args request={.function=0x80ccu};
    if(port_v37_sysqio_preflight(&request,3u,access_limit,&device,&action)!=
       PORT_V37_SYSQIO_PRECHECK_OK||action.full_function!=0x80ccu||
       action.function_code!=PORT_DQ_READPBLK)return 7;
    request.function=PORT_DQ_READLBLK;
    access_limit=3u;
    memset(&action,0x5a,sizeof action);
    struct port_v37_sysqio_dispatch saved=action;
    if(port_v37_sysqio_preflight(&request,3u,access_limit,&device,&action)!=
       PORT_V37_SYSQIO_PRECHECK_NOPRIV||memcmp(&action,&saved,sizeof action))
        return 8;
    access_limit=4u;device.online=0u;
    if(port_v37_sysqio_preflight(&request,3u,access_limit,&device,&action)!=
       PORT_V37_SYSQIO_PRECHECK_OFFLINE)return 9;
    request.function=PORT_DQ_DEACCESS;
    if(port_v37_sysqio_preflight(&request,3u,access_limit,&device,&action)!=
       PORT_V37_SYSQIO_PRECHECK_OK)return 10;
    request.function=63u;
    if(port_v37_sysqio_preflight(&request,3u,access_limit,&device,&action)!=
       PORT_V37_SYSQIO_PRECHECK_ILLFUNC)return 11;
    uint32_t local[2]={UINT32_MAX,UINT32_MAX};uint32_t service_status=0u;
    if(port_v37_sysqio_clear_efn(0x1003u,local,clear_global,0,
         &service_status)!=PORT_V37_SYSQIO_EF_OK||
       local[0]!=(UINT32_MAX&~(UINT32_C(1)<<3))||local[1]!=UINT32_MAX||
       service_status!=PORT_SYS85_NORMAL)return 12;
    local[0]=local[1]=UINT32_MAX;
    if(port_v37_sysqio_clear_efn(63u,local,clear_global,0,
         &service_status)!=PORT_V37_SYSQIO_EF_OK||local[0]!=UINT32_MAX||
       local[1]!=(UINT32_MAX&~UINT32_C(0x80000000)))
        return 13;
    struct global_ef_test global={0u,PORT_SYS85_NORMAL};
    if(port_v37_sysqio_clear_efn(64u,local,clear_global,&global,
         &service_status)!=PORT_V37_SYSQIO_EF_OK||global.seen!=64u||
       local[0]!=UINT32_MAX||local[1]!=(UINT32_MAX&~UINT32_C(0x80000000)))
        return 14;
    global.status=PORT_SYS85_NOPRIV;
    if(port_v37_sysqio_clear_efn(0x1ffu,local,clear_global,&global,
         &service_status)!=PORT_V37_SYSQIO_EF_GLOBAL_FAILURE||
       global.seen!=255u||service_status!=PORT_SYS85_NOPRIV)return 15;
    struct iosb_memory_test mem={.base=0x1000u};
    memset(mem.bytes,0xa5,sizeof mem.bytes);
    const struct port_v37_sysqio_memory memops={
        probe_iosb,clear_iosb,&mem};
    if(port_v37_sysqio_clear_iosb(0x1008u,&memops)!=
       PORT_V37_SYSQIO_IOSB_OK||mem.probes!=1u||mem.clears!=1u)
        return 16;
    for(unsigned i=8;i<16;i++)if(mem.bytes[i]!=0u)return 17;
    if(mem.bytes[7]!=0xa5u||mem.bytes[16]!=0xa5u)return 18;
    mem.deny_probe=1;mem.probes=mem.clears=0u;
    memset(mem.bytes,0xa5,sizeof mem.bytes);
    if(port_v37_sysqio_clear_iosb(0x1008u,&memops)!=
       PORT_V37_SYSQIO_IOSB_ACCVIO||mem.probes!=1u||mem.clears!=0u||
       mem.bytes[8]!=0xa5u)return 19;
    mem.deny_probe=0;mem.probes=mem.clears=0u;
    if(port_v37_sysqio_clear_iosb(0u,&memops)!=
       PORT_V37_SYSQIO_IOSB_OK||mem.probes||mem.clears)return 20;
    device.online=1u;
    struct begin_rig rig={.device=&device,.ucb=(void *)(uintptr_t)0x2000u,
        .expected_pid=7u,.expected_channel=0x12340020u};
    struct port_v37_sysqio_process proc={.pid=7u,.access_mode=3u,
        .has_phy_io=1u,
        .local_event_flags={UINT32_MAX,UINT32_MAX}};
    const struct port_v37_sysqio_begin_ops begin_ops={
        .context=&rig,.lookup_channel=lookup_channel,.post_event=post_event,
        .single_io_quota=single_quota,.user_memory=memops};
    struct port_v37_sysqio_prepared prepared={0};
    struct port_v37_sysqio_prepared untouched;
    uint32_t begin_status=0u;
    proc.direct_io_count=1;
    memset(mem.bytes,0xa5,sizeof mem.bytes);mem.probes=mem.clears=0u;
    if(port_v37_sysqio_begin(ap,sizeof ap,&proc,&begin_ops,&prepared,
         &begin_status)!=PORT_V37_SYSQIO_BEGIN_OK||
       begin_status!=PORT_V37_SYSQIO_BEGIN_OK||rig.lookups!=1u||rig.posts||
       proc.local_event_flags[0]!=(UINT32_MAX&~(1u<<3))||
       proc.local_event_flags[1]!=UINT32_MAX||
       prepared.channel.ucb!=rig.ucb||prepared.dispatch.function_code!=49u||
       mem.probes!=1u||mem.clears!=1u||proc.direct_io_count!=0)
        return 21;
    _Alignas(4) uint8_t ccb_table[64]={0};
    uint8_t *const ccb_base=ccb_table+48u;
    uint8_t *const source_ccb=ccb_base-32u;
    put32(source_ccb+PORT_V37_CCB_UCB,0x2000u);
    put32(source_ccb+PORT_V37_CCB_WIND,0u);
    source_ccb[PORT_V37_CCB_AMOD]=4u;
    rig.source_ucb=0x2000u;rig.lookups=rig.posts=0u;
    struct port_v37_sysqio_begin_ops source_ccb_ops={
        .context=&rig,.ccb_base=ccb_base,.ccb_preceding_bytes=48u,
        .ccb_maximum_channel_offset=48u,.lookup_ucb=lookup_ucb,
        .post_event=post_event,.single_io_quota=single_quota,
        .user_memory=memops};
    put32(ap+8u,32u);proc.direct_io_count=1;
    proc.local_event_flags[0]=UINT32_MAX;
    mem.probes=mem.clears=0u;
    if(port_v37_sysqio_begin(ap,sizeof ap,&proc,&source_ccb_ops,&prepared,
         &begin_status)!=PORT_V37_SYSQIO_BEGIN_OK||rig.lookups!=1u||
       prepared.channel.ucb!=rig.ucb||prepared.channel.ccb_access_limit!=4u||
       prepared.channel.window!=0u||proc.direct_io_count!=0)
        return 59;
    source_ccb[PORT_V37_CCB_AMOD]=3u;rig.lookups=rig.posts=0u;
    proc.direct_io_count=1;
    if(port_v37_sysqio_begin(ap,sizeof ap,&proc,&source_ccb_ops,&prepared,
         &begin_status)!=PORT_V37_SYSQIO_BEGIN_NOPRIV||
       begin_status!=PORT_SYS85_NOPRIV||rig.lookups||rig.posts!=1u||
       proc.direct_io_count!=1)
        return 60;
    source_ccb[PORT_V37_CCB_AMOD]=4u;
    rig.source_ccb=source_ccb;
    put32(source_ccb+PORT_V37_CCB_WIND,1u);rig.lookups=0u;
    if(port_v37_sysqio_begin(ap,sizeof ap,&proc,&source_ccb_ops,&prepared,
         &begin_status)!=PORT_V37_SYSQIO_BEGIN_WAIT||rig.lookups!=1u||
       proc.direct_io_count!=1)
        return 61;
    source_ccb_ops.wait_channel=wait_source_channel;
    rig.waits=0u;proc.direct_io_count=1;
    if(port_v37_sysqio_begin(ap,sizeof ap,&proc,&source_ccb_ops,&prepared,
         &begin_status)!=PORT_V37_SYSQIO_BEGIN_OK||rig.waits!=1u||
       rig.lookups!=3u||prepared.channel.window!=0u||
       proc.direct_io_count!=0)
        return 62;
    source_ccb_ops.wait_channel=0;rig.posts=rig.lookups=0u;
    proc.direct_io_count=1;put32(ap+8u,0u);
    if(port_v37_sysqio_begin(ap,sizeof ap,&proc,&source_ccb_ops,&prepared,
         &begin_status)!=PORT_V37_SYSQIO_BEGIN_IVCHAN||
       begin_status!=PORT_SYS85_IVCHAN||rig.posts!=1u||rig.lookups||
       proc.direct_io_count!=1)
        return 63;
    put32(ap+8u,0x12340020u);
    rig.window=1u;rig.posts=0u;rig.waits=0u;proc.direct_io_count=1;
    mem.probes=mem.clears=0u;
    memset(&prepared,0x5a,sizeof prepared);untouched=prepared;
    begin_status=0xfeedu;
    if(port_v37_sysqio_begin(ap,sizeof ap,&proc,&begin_ops,&prepared,
         &begin_status)!=PORT_V37_SYSQIO_BEGIN_WAIT||rig.posts||
       rig.waits||proc.direct_io_count!=1||mem.probes||mem.clears||
       begin_status!=0xfeedu||memcmp(&prepared,&untouched,sizeof prepared))
        return 57;
    struct port_v37_sysqio_begin_ops wait_ops=begin_ops;
    wait_ops.wait_channel=wait_channel;
    if(port_v37_sysqio_begin(ap,sizeof ap,&proc,&wait_ops,&prepared,
         &begin_status)!=PORT_V37_SYSQIO_BEGIN_OK||rig.waits!=1u||
       prepared.channel.window!=0u||proc.direct_io_count!=0)
        return 58;
    proc.direct_io_count=0;rig.window=0u;rig.posts=0u;
    rig.deny_channel=1;rig.posts=0u;rig.lookups=0u;
    memset(mem.bytes,0xa5,sizeof mem.bytes);mem.probes=mem.clears=0u;
    memset(&untouched,0x5a,sizeof untouched);prepared=untouched;
    if(port_v37_sysqio_begin(ap,sizeof ap,&proc,&begin_ops,&prepared,
         &begin_status)!=PORT_V37_SYSQIO_BEGIN_IVCHAN||
       begin_status!=PORT_SYS85_IVCHAN||rig.lookups!=1u||
       rig.posts!=1u||rig.posted_pid!=7u||rig.posted_efn!=3u||
       rig.posted_status!=PORT_SYS85_IVCHAN||
       mem.probes||mem.clears||memcmp(&prepared,&untouched,sizeof prepared))
        return 22;
    rig.deny_channel=0;rig.posts=0u;rig.ccb_access_limit=3u;
    if(port_v37_sysqio_begin(ap,sizeof ap,&proc,&begin_ops,&prepared,
         &begin_status)!=PORT_V37_SYSQIO_BEGIN_NOPRIV||
       begin_status!=PORT_SYS85_NOPRIV||rig.posts!=1u||
       rig.posted_status!=PORT_SYS85_NOPRIV||proc.direct_io_count!=0)
        return 23;
    rig.posts=0u;rig.ccb_access_limit=4u;
    put32(ap+12u,63u);
    if(port_v37_sysqio_begin(ap,sizeof ap,&proc,&begin_ops,&prepared,
         &begin_status)!=PORT_V37_SYSQIO_BEGIN_ILLFUNC||
       begin_status!=PORT_SYS85_ILLIOFUNC||rig.posts!=1u||
       rig.posted_status!=PORT_SYS85_ILLIOFUNC||proc.direct_io_count!=0)
        return 24;
    rig.posts=0u;device.online=0u;put32(ap+12u,PORT_DQ_READLBLK);
    if(port_v37_sysqio_begin(ap,sizeof ap,&proc,&begin_ops,&prepared,
         &begin_status)!=PORT_V37_SYSQIO_BEGIN_OFFLINE||
       begin_status!=PORT_SYS85_DEVOFFLINE||rig.posts!=1u||
       rig.posted_status!=PORT_SYS85_DEVOFFLINE||proc.direct_io_count!=0)
        return 25;
    device.online=1u;put32(ap+12u,values[2]);
    rig.ccb_access_limit=4u;rig.posts=0u;rig.quota_calls=0u;rig.grant_quota=0;
    memset(mem.bytes,0xa5,sizeof mem.bytes);mem.probes=mem.clears=0u;
    if(port_v37_sysqio_begin(ap,sizeof ap,&proc,&begin_ops,&prepared,
         &begin_status)!=PORT_V37_SYSQIO_BEGIN_EXQUOTA||
       begin_status!=PORT_SS_EXQUOTA||rig.quota_calls!=1u||
       rig.quota_buffered||rig.posts!=1u||rig.posted_status!=PORT_SS_EXQUOTA||
       proc.direct_io_count!=0||mem.probes!=1u||mem.clears!=1u||
       mem.bytes[0]!=0u||memcmp(&prepared,&untouched,sizeof prepared)){
        return 26;
    }
    rig.grant_quota=1;rig.posts=0u;
    if(port_v37_sysqio_begin(ap,sizeof ap,&proc,&begin_ops,&prepared,
         &begin_status)!=PORT_V37_SYSQIO_BEGIN_OK||
       begin_status!=PORT_V37_SYSQIO_BEGIN_OK||rig.quota_calls!=2u||
       proc.direct_io_count!=-1||prepared.dispatch.function_code!=49u)
        return 27;
    uint8_t irp[PORT_V37_SYSQIO_IRP_STANDARD_BYTES+16u];
    memset(irp,0xa5,sizeof irp);
    const struct port_v37_sysqio_irp_spec ispec={
        .packet_va=0x4000u,.ucb_va=0x2000u,.arb_va=0x3000u,
        .window_va=0u,.channel_index=32u,.base_priority=8u};
    if(port_v37_sysqio_build_irp(&prepared,&proc,&ispec,irp,sizeof irp)!=
         PORT_V37_SYSQIO_IRP_BUILD_OK||
       get16(irp+8u)!=PORT_V37_SYSQIO_IRP_STANDARD_BYTES||
       irp[10u]!=PORT_V37_DYN_IRP||irp[11u]!=0x43u||
       get32(irp+12u)!=7u||get32(irp+16u)!=0x2000u||
       get32(irp+20u)!=0xaabbccddu||get32(irp+24u)!=0u||
       get32(irp+28u)!=0x2000u||get16(irp+32u)!=0x31u||
       irp[34u]!=3u||irp[35u]!=8u||get32(irp+36u)!=0x1000u||
       get16(irp+40u)!=(uint16_t)(0u-32u)||get16(irp+42u)!=2u||
       get32(irp+44u)!=0u||get16(irp+48u)!=0u||
       get32(irp+52u)!=2u||irp[64u]!=4u||
       get32(irp+PORT_V37_IRP_ARB)!=0x3000u||
       irp[PORT_V37_SYSQIO_IRP_STANDARD_BYTES]!=0xa5u){
        return 28;
    }
    uint32_t ast_status=0u;
    if(proc.ast_count!=-1||port_v37_sysqio_check_ast_quota(&prepared,&proc,
          &ast_status)!=PORT_V37_SYSQIO_AST_EXQUOTA||
       ast_status!=PORT_SS_EXQUOTA)
        return 36;
    enum { QIO_POOL_BASE=0x620000u,QIO_POOL_SENTINEL=0x621000u,
           QIO_POOL_STRIDE=144u };
    uint8_t pool_memory[512u];
    memset(pool_memory,0xa5,sizeof pool_memory);
    const struct port_ini_packet_area pool_area={.memory=pool_memory,
        .address=QIO_POOL_BASE,.available_bytes=sizeof pool_memory,
        .count=2u,.packet_bytes=QIO_POOL_STRIDE,
        .minimum_bytes=QIO_POOL_STRIDE,.sentinel_address=QIO_POOL_SENTINEL};
    struct port_ini_packet_list pool={0};
    if(port_ini_packet_init(&pool_area,&pool)!=PORT_INI_PACKETS_OK)return 45;
    struct pool_sync_state pool_state={.ipl=1u};
    const struct port_v37_irp_pool_sync pool_sync={&pool_state,pool_raise,
        pool_restore};
    struct port_v37_sysqio_irp_spec reserve_spec={.ucb_va=0x2000u,
        .arb_va=0x3000u,.channel_index=32u,.base_priority=8u};
    uint8_t *reserved_irp=0;uint32_t reserved_va=0;
    proc.ast_count=1;
    if(port_v37_sysqio_reserve_irp(&prepared,&proc,&reserve_spec,&pool,
          &pool_sync,&reserved_irp,&reserved_va,&ast_status)!=
          PORT_V37_SYSQIO_RESERVE_OK||ast_status!=PORT_SYS85_NORMAL||
       reserved_va!=QIO_POOL_BASE+QIO_POOL_STRIDE||
       reserved_irp!=pool_memory+QIO_POOL_STRIDE||pool.free_count!=1u||
       proc.ast_count!=0||get32(reserved_irp+12u)!=proc.pid)
        return 46;
    if(port_v37_irp_pool_free(&pool,reserved_irp,reserved_va,
          PORT_V37_SYSQIO_IRP_STANDARD_BYTES,&pool_sync)!=PORT_V37_IRP_POOL_OK)
        return 49;
    struct port_v37_sysqio_prepared one_parameter=prepared;
    one_parameter.args.function=PORT_DQ_SEEK;
    one_parameter.args.parameter[0]=0x12345678u;
    if(port_v37_sysqio_preflight(&one_parameter.args,proc.access_mode,4u,
          &device,&one_parameter.dispatch)!=PORT_V37_SYSQIO_PRECHECK_OK||
       one_parameter.dispatch.action_class!=PORT_DQ_FDT_ONE||
       one_parameter.dispatch.function_code!=PORT_DQ_SEEK)
        return 70;
    uint8_t one_parameter_irp[PORT_V37_SYSQIO_IRP_STANDARD_BYTES];
    struct port_v37_sysqio_irp_spec one_parameter_spec=reserve_spec;
    one_parameter_spec.packet_va=0x4000u;
    if(port_v37_sysqio_build_irp(&one_parameter,&proc,&one_parameter_spec,
          one_parameter_irp,sizeof one_parameter_irp)!=
          PORT_V37_SYSQIO_IRP_BUILD_OK||
       get32(one_parameter_irp+PORT_V37_IRP_MEDIA)!=0x12345678u)
        return 67;
    struct port_v37_sysqio_prepared read_request=prepared;
    read_request.args.function=PORT_DQ_READPBLK;
    read_request.args.parameter[0]=0x12345ff0u;
    read_request.args.parameter[1]=0x1234ffffu;
    read_request.args.parameter[3]=0xabcdef80u;
    if(port_v37_sysqio_preflight(&read_request.args,proc.access_mode,4u,
          &device,&read_request.dispatch)!=PORT_V37_SYSQIO_PRECHECK_OK||
       read_request.dispatch.action_class!=PORT_DQ_FDT_READ||
       port_v37_sysqio_build_irp(&read_request,&proc,&one_parameter_spec,
          one_parameter_irp,sizeof one_parameter_irp)!=
          PORT_V37_SYSQIO_IRP_BUILD_OK||
       !(get16(one_parameter_irp+42u)&2u)||
       get32(one_parameter_irp+PORT_V37_IRP_BCNT)!=0xffffu||
       one_parameter_irp[64u]!=0x80u)
        return 71;
    struct transfer_access_test transfer_test={.read_ok=1,.write_ok=1};
    const struct port_exe_access transfer_access={&transfer_test,0,0,
        probe_transfer_read,probe_transfer_write,0};
    struct port_v37_sysqio_buffer transfer_buffer={0};
    if(port_v37_sysqio_check_buffer(&read_request,&transfer_access,
          &transfer_buffer,&ast_status)!=PORT_V37_SYSQIO_BUFFER_OK||
       ast_status!=PORT_SYS85_NORMAL||transfer_test.writes!=1u||
       transfer_test.reads||transfer_test.address!=0x12345ff0u||
       transfer_test.bytes!=0xffffu||
       transfer_buffer.address!=0x12345ff0u||transfer_buffer.bytes!=0xffffu||
       transfer_buffer.direction!=PORT_V37_SYSQIO_BUFFER_WRITE_TO_USER)
        return 73;
    transfer_test.write_ok=0;transfer_buffer.address=0xdeadbeefu;
    if(port_v37_sysqio_check_buffer(&read_request,&transfer_access,
          &transfer_buffer,&ast_status)!=PORT_V37_SYSQIO_BUFFER_ACCVIO||
       ast_status!=PORT_SYS85_ACCVIO||
       transfer_buffer.address!=0xdeadbeefu)
        return 74;
    struct port_v37_sysqio_prepared write_request=read_request;
    write_request.args.function=PORT_DQ_WRITEPBLK;
    if(port_v37_sysqio_preflight(&write_request.args,proc.access_mode,4u,
          &device,&write_request.dispatch)!=PORT_V37_SYSQIO_PRECHECK_OK||
       write_request.dispatch.action_class!=PORT_DQ_FDT_WRITE||
       port_v37_sysqio_build_irp(&write_request,&proc,&one_parameter_spec,
          one_parameter_irp,sizeof one_parameter_irp)!=
          PORT_V37_SYSQIO_IRP_BUILD_OK||
       (get16(one_parameter_irp+42u)&2u)||
       get32(one_parameter_irp+PORT_V37_IRP_BCNT)!=0xffffu||
       one_parameter_irp[64u]!=0x80u)
        return 72;
    transfer_test.write_ok=1;transfer_test.reads=transfer_test.writes=0u;
    if(port_v37_sysqio_check_buffer(&write_request,&transfer_access,
          &transfer_buffer,&ast_status)!=PORT_V37_SYSQIO_BUFFER_OK||
       transfer_test.reads!=1u||transfer_test.writes||
       transfer_buffer.direction!=PORT_V37_SYSQIO_BUFFER_READ_FROM_USER)
        return 75;
    write_request.args.parameter[1]=0x10000u;
    transfer_test.reads=transfer_test.writes=0u;
    if(port_v37_sysqio_check_buffer(&write_request,&transfer_access,
          &transfer_buffer,&ast_status)!=PORT_V37_SYSQIO_BUFFER_OK||
       transfer_test.reads||transfer_test.writes||transfer_buffer.bytes!=0u)
        return 76;
    write_request.args.function=PORT_DQ_MODIFY;
    if(port_v37_sysqio_preflight(&write_request.args,proc.access_mode,4u,
          &device,&write_request.dispatch)!=PORT_V37_SYSQIO_PRECHECK_OK||
       write_request.dispatch.action_class!=PORT_DQ_FDT_MODIFY)
        return 82;
    transfer_test.reads=transfer_test.writes=0u;
    if(port_v37_sysqio_check_buffer(&write_request,&transfer_access,
          &transfer_buffer,&ast_status)!=PORT_V37_SYSQIO_BUFFER_NONE||
       transfer_test.reads||transfer_test.writes)
        return 80;
    one_parameter_spec.packet_va=0x4000u;
    if(port_v37_sysqio_build_irp(&write_request,&proc,&one_parameter_spec,
          one_parameter_irp,sizeof one_parameter_irp)!=
          PORT_V37_SYSQIO_IRP_BUILD_OK||
       get32(one_parameter_irp+PORT_V37_IRP_BCNT)!=0u||
       one_parameter_irp[64u]!=0u)
        return 81;
    transfer_pde[0]=0x1007u;
    for(unsigned i=1u;i<=16u;i++)
        transfer_pte[i]=((i+1u)<<12)|7u;
    struct port_user_map transfer_map={transfer_pde,resolve_transfer_table,0};
    struct port_exe_access mapped_access;
    struct port_exe_user_access mapped_context;
    port_exe_user_access_init(&mapped_access,&mapped_context,&transfer_map,
                              resolve_transfer_page,0);
    struct port_v37_sysqio_prepared mapped_request=read_request;
    mapped_request.args.parameter[0]=0x1001u;
    mapped_request.args.parameter[1]=UINT16_MAX;
    if(port_v37_sysqio_check_buffer(&mapped_request,&mapped_access,
          &transfer_buffer,&ast_status)!=PORT_V37_SYSQIO_BUFFER_OK||
       transfer_buffer.bytes!=UINT16_MAX)
        return 77;
    transfer_pte[16]=0x11005u;transfer_buffer.address=0xdeadbeefu;
    if(port_v37_sysqio_check_buffer(&mapped_request,&mapped_access,
          &transfer_buffer,&ast_status)!=PORT_V37_SYSQIO_BUFFER_ACCVIO||
       transfer_buffer.address!=0xdeadbeefu)
        return 78;
    mapped_request.dispatch.action_class=PORT_DQ_FDT_WRITE;
    if(port_v37_sysqio_check_buffer(&mapped_request,&mapped_access,
          &transfer_buffer,&ast_status)!=PORT_V37_SYSQIO_BUFFER_OK||
       transfer_buffer.direction!=PORT_V37_SYSQIO_BUFFER_READ_FROM_USER)
        return 79;
    uint8_t *prepared_irp=0;uint32_t prepared_va=0u;
    struct port_v37_sysqio_prepared integrated={0};
    proc.direct_io_count=1;proc.ast_count=1;rig.posts=0u;
    rig.window=1u;integrated=untouched;prepared_irp=(uint8_t *)(uintptr_t)0x55u;
    prepared_va=0x55u;ast_status=0xfeedu;
    if(port_v37_sysqio_prepare_irp(ap,sizeof ap,&proc,&begin_ops,
          &reserve_spec,&pool,&pool_sync,&integrated,&prepared_irp,
          &prepared_va,&ast_status)!=PORT_V37_SYSQIO_PREPARE_WAIT||
       rig.posts||proc.direct_io_count!=1||proc.ast_count!=1||
       prepared_irp!=(uint8_t *)(uintptr_t)0x55u||prepared_va!=0x55u||
       ast_status!=0xfeedu||memcmp(&integrated,&untouched,sizeof integrated))
        return 59;
    rig.window=0u;
    prepared_irp=0;prepared_va=0u;
    if(port_v37_sysqio_prepare_irp(ap,sizeof ap,&proc,&begin_ops,
          &reserve_spec,&pool,&pool_sync,&integrated,&prepared_irp,
          &prepared_va,&ast_status)!=PORT_V37_SYSQIO_PREPARE_OK||
       ast_status!=PORT_SYS85_NORMAL||proc.direct_io_count!=0||
       proc.ast_count!=0||!prepared_irp||!prepared_va||rig.posts||
       integrated.dispatch.function_code!=49u)
        return 50;
    if(port_v37_irp_pool_free(&pool,prepared_irp,prepared_va,
          PORT_V37_SYSQIO_IRP_STANDARD_BYTES,&pool_sync)!=PORT_V37_IRP_POOL_OK)
        return 51;
    /* SYSQIOREQ ACCESS follows packet and AST reservation. A failed
     * physical-I/O privilege check must return that packet and both charges. */
    put32(ap+12u,PORT_DQ_READPBLK);
    proc.direct_io_count=1;proc.ast_count=1;proc.has_phy_io=0u;
    rig.posts=0u;prepared_irp=0;prepared_va=0u;integrated=untouched;
    if(port_v37_sysqio_prepare_irp(ap,sizeof ap,&proc,&begin_ops,
          &reserve_spec,&pool,&pool_sync,&integrated,&prepared_irp,
          &prepared_va,&ast_status)!=PORT_V37_SYSQIO_PREPARE_ACCESS||
       ast_status!=PORT_SYS85_NOPRIV||proc.direct_io_count!=1||
       proc.ast_count!=1||pool.free_count!=2u||rig.posts!=1u||
       rig.posted_status!=PORT_SYS85_NOPRIV||prepared_irp||prepared_va||
       memcmp(&integrated,&untouched,sizeof integrated))
        return 64;
    proc.has_phy_io=1u;rig.posts=0u;
    if(port_v37_sysqio_prepare_irp(ap,sizeof ap,&proc,&begin_ops,
          &reserve_spec,&pool,&pool_sync,&integrated,&prepared_irp,
          &prepared_va,&ast_status)!=PORT_V37_SYSQIO_PREPARE_OK||
       !prepared_irp||integrated.dispatch.physical_io!=1u||
       !(get16(prepared_irp+42u)&0x0100u)||proc.direct_io_count!=0||
       proc.ast_count!=0||rig.posts)
        return 65;
    if(port_v37_irp_pool_free(&pool,prepared_irp,prepared_va,
          PORT_V37_SYSQIO_IRP_STANDARD_BYTES,&pool_sync)!=PORT_V37_IRP_POOL_OK)
        return 66;
    put32(ap+12u,PORT_DQ_READVBLK);
    proc.direct_io_count=1;proc.ast_count=1;
    _Alignas(4) uint8_t source_pcb[PORT_V37_PCB_BYTES]={0};
    put16(source_pcb+PORT_V37_PCB_SIZE,PORT_V37_PCB_BYTES);
    source_pcb[PORT_V37_PCB_BTYPE]=PORT_V37_DYN_PCB;
    put16(source_pcb+PORT_V37_PCB_ASTCNT,1u);
    put16(source_pcb+PORT_V37_PCB_BIOCNT,2u);
    put16(source_pcb+PORT_V37_PCB_DIOCNT,1u);
    put32(source_pcb+PORT_V37_PCB_EFCS,UINT32_MAX);
    put32(source_pcb+PORT_V37_PCB_EFCU,UINT32_MAX);
    put32(source_pcb+PORT_V37_PCB_PID,7u);
    put32(source_pcb+PORT_V37_PCB_PHD,0x2000u);
    _Alignas(4) uint8_t source_phd[PORT_V37_PHD_PRIVMSK_BYTES]={0};
    source_phd[PORT_V37_PRIV_LOG_IO>>3]|=
        (uint8_t)(1u<<(PORT_V37_PRIV_LOG_IO&7u));
    source_phd[PORT_V37_PRIV_PHY_IO>>3]|=
        (uint8_t)(1u<<(PORT_V37_PRIV_PHY_IO&7u));
    struct port_v37_sysqio_process pcb_process={0};
    if(port_v37_sysqio_process_bind_pcb(source_pcb,sizeof source_pcb,
          source_phd,sizeof source_phd,0x2000u,3u,&pcb_process)!=
          PORT_V37_SYSQIO_PCB_OK||pcb_process.pid!=7u||
       !pcb_process.has_phy_io||!pcb_process.has_log_io||
       port_v37_sysqio_process_io_count(&pcb_process,
          PORT_V37_SYSQIO_PCB_DIRECT)!=1||
       port_v37_sysqio_process_io_count(&pcb_process,
          PORT_V37_SYSQIO_PCB_BUFFERED)!=2)
        return 60;
    struct port_v37_sysqio_process unchanged_process=pcb_process;
    if(port_v37_sysqio_process_bind_pcb(source_pcb,sizeof source_pcb,
          source_phd,sizeof source_phd,0x2004u,3u,&pcb_process)!=
          PORT_V37_SYSQIO_PCB_BAD||
       memcmp(&pcb_process,&unchanged_process,sizeof pcb_process))
        return 68;
    memset(source_phd,0,sizeof source_phd);
    if(port_v37_sysqio_process_bind_pcb(source_pcb,sizeof source_pcb,
          source_phd,sizeof source_phd,0x2000u,3u,&pcb_process)!=
          PORT_V37_SYSQIO_PCB_OK||pcb_process.has_phy_io||
       pcb_process.has_log_io)
        return 67;
    rig.window=0u;rig.posts=0u;proc.direct_io_count=1;proc.ast_count=1;
    if(port_v37_sysqio_prepare_irp(ap,sizeof ap,&pcb_process,&begin_ops,
          &reserve_spec,&pool,&pool_sync,&integrated,&prepared_irp,
          &prepared_va,&ast_status)!=PORT_V37_SYSQIO_PREPARE_OK||
       pcb_process.pid!=7u||
       port_v37_sysqio_process_io_count(&pcb_process,
          PORT_V37_SYSQIO_PCB_DIRECT)!=0||
       port_v37_sysqio_process_ast_count(&pcb_process)!=0||
       get16(source_pcb+PORT_V37_PCB_DIOCNT)!=0u||
       get16(source_pcb+PORT_V37_PCB_ASTCNT)!=0u||
       get32(source_pcb+PORT_V37_PCB_EFCS)!=(UINT32_MAX&~(1u<<3))||
       get32(source_pcb+PORT_V37_PCB_EFCU)!=UINT32_MAX)
        return 61;
    if(port_v37_irp_pool_free(&pool,prepared_irp,prepared_va,
          PORT_V37_SYSQIO_IRP_STANDARD_BYTES,&pool_sync)!=PORT_V37_IRP_POOL_OK)
        return 62;
    proc.direct_io_count=1;proc.ast_count=1;rig.posts=0u;
    reserve_spec.ucb_va=0u;
    if(port_v37_sysqio_prepare_irp(ap,sizeof ap,&proc,&begin_ops,
          &reserve_spec,&pool,&pool_sync,&integrated,&reserved_irp,
          &reserved_va,&ast_status)!=PORT_V37_SYSQIO_PREPARE_RESERVE||
       ast_status!=PORT_SYS85_BADPARAM||proc.direct_io_count!=1||
       proc.ast_count!=1||rig.posts!=1u||rig.posted_efn!=3u||
       rig.posted_status!=PORT_SYS85_BADPARAM||pool.free_count!=2u)
        return 53;
    reserve_spec.ucb_va=0x2000u;
    /* Source AST quota failure is after allocation and must restore the
     * same lookaside packet without consuming another AST unit. */
    proc.ast_count=-1;
    reserved_irp=0;reserved_va=0;
    uint32_t reserve_status=port_v37_sysqio_reserve_irp(&prepared,&proc,
          &reserve_spec,&pool,&pool_sync,&reserved_irp,&reserved_va,&ast_status);
    if(reserve_status!=
          PORT_V37_SYSQIO_RESERVE_ASTQUOTA||ast_status!=PORT_SS_EXQUOTA||
       reserved_irp||reserved_va||pool.free_count!=2u||proc.ast_count!=-1){
        return 47;
    }
    proc.ast_count=1;
    reserve_spec.ucb_va=0u;
    if(port_v37_sysqio_reserve_irp(&prepared,&proc,&reserve_spec,&pool,
          &pool_sync,&reserved_irp,&reserved_va,&ast_status)!=
          PORT_V37_SYSQIO_RESERVE_BAD||ast_status!=PORT_SYS85_BADPARAM||
       reserved_irp||reserved_va||pool.free_count!=2u||proc.ast_count!=1)
        return 48;
    struct port_v37_sysqio_prepared no_ast=prepared;
    no_ast.args.ast_address=0u;proc.ast_count=-1;
    if(port_v37_sysqio_check_ast_quota(&no_ast,&proc,&ast_status)!=
          PORT_V37_SYSQIO_AST_OK||ast_status!=PORT_SYS85_NORMAL)
        return 37;
    memset(irp,0x5a,sizeof irp);
    struct port_v37_sysqio_irp_spec bad_spec=ispec;
    bad_spec.packet_va=3u;
    if(port_v37_sysqio_build_irp(&prepared,&proc,&bad_spec,irp,sizeof irp)!=
         PORT_V37_SYSQIO_IRP_BUILD_BAD||irp[0]!=0x5au||irp[sizeof irp-1u]!=0x5au)
        return 29;
    bad_spec=ispec;bad_spec.channel_index=33u;
    if(port_v37_sysqio_build_irp(&prepared,&proc,&bad_spec,irp,sizeof irp)!=
         PORT_V37_SYSQIO_IRP_BUILD_BAD||irp[0]!=0x5au)
        return 69;
    bad_spec=ispec;bad_spec.channel_index=0u;
    if(port_v37_sysqio_build_irp(&prepared,&proc,&bad_spec,irp,sizeof irp)!=
         PORT_V37_SYSQIO_IRP_BUILD_BAD||irp[0]!=0x5au)
        return 70;
    struct port_v37_sysqio_prepared mismatched_channel=prepared;
    mismatched_channel.args.channel=0x12340030u;
    if(port_v37_sysqio_build_irp(&mismatched_channel,&proc,&ispec,irp,
          sizeof irp)!=PORT_V37_SYSQIO_IRP_BUILD_BAD||irp[0]!=0x5au)
        return 71;
    uint16_t normalized=0xaaaau;uint8_t check_ast=0xaau;
    uint32_t access_status=0xfeedu;
    if(port_v37_sysqio_normalize_write(0x0fb0u,0u,0u,&normalized,
          &check_ast,&access_status)!=PORT_V37_SYSQIO_ACCESS_OK||
       normalized!=0x0fa0u||check_ast!=1u||access_status!=PORT_SYS85_NORMAL)
        return 30;
    if(port_v37_sysqio_normalize_write(PORT_DQ_WRITEVBLK,
          PORT_V37_DEVCHAR_FOD,0u,&normalized,&check_ast,&access_status)!=
          PORT_V37_SYSQIO_ACCESS_NOPRIV||access_status!=PORT_SYS85_NOPRIV||
       normalized!=0x0fa0u||check_ast!=1u)
        return 31;
    if(port_v37_sysqio_normalize_write(PORT_DQ_WRITEVBLK,
          PORT_V37_DEVCHAR_FOD|PORT_V37_DEVCHAR_MNT,0u,&normalized,
          &check_ast,&access_status)!=PORT_V37_SYSQIO_ACCESS_OK||
       normalized!=PORT_DQ_WRITEVBLK||check_ast!=0u)
        return 32;
    if(port_v37_sysqio_normalize_write(PORT_DQ_WRITEVBLK,
          PORT_V37_DEVCHAR_FOD|PORT_V37_DEVCHAR_MNT|PORT_V37_DEVCHAR_FOR,
          0u,&normalized,&check_ast,&access_status)!=PORT_V37_SYSQIO_ACCESS_OK||
       normalized!=PORT_DQ_WRITELBLK||check_ast!=0u)
        return 33;
    if(port_v37_sysqio_normalize_write(PORT_DQ_WRITEVBLK,
          PORT_V37_DEVCHAR_SPL,0u,&normalized,&check_ast,&access_status)!=
          PORT_V37_SYSQIO_ACCESS_OK||normalized!=PORT_DQ_WRITELBLK||check_ast)
        return 34;
    if(port_v37_sysqio_normalize_write(PORT_DQ_READVBLK,0u,0u,
          &normalized,&check_ast,&access_status)!=PORT_V37_SYSQIO_ACCESS_OK||
       normalized!=PORT_DQ_READVBLK||check_ast)
        return 35;
    struct volume_access_test vat={.status=PORT_SYS85_NORMAL};
    uint8_t physical=0xaau;
    if(port_v37_sysqio_device_access(PORT_DQ_READPBLK,
          PORT_V37_DEVCHAR_SHR,0u,0u,0x1234u,0x55667788u,0x3000u,
          check_volume,&vat,&physical,&access_status)!=
          PORT_V37_SYSQIO_DEVICE_NOPRIV||access_status!=PORT_SYS85_NOPRIV||
       vat.calls||physical!=0xaau)
        return 38;
    if(port_v37_sysqio_device_access(PORT_DQ_READPBLK,
          PORT_V37_DEVCHAR_SHR,0u,1u,0x1234u,0x55667788u,0x3000u,
          check_volume,&vat,&physical,&access_status)!=
          PORT_V37_SYSQIO_DEVICE_OK||vat.calls!=1u||
       vat.kind!=PORT_V37_SYSQIO_ACCESS_PHYSICAL||vat.protection!=0x1234u||
       vat.owner!=0x55667788u||vat.arb!=0x3000u||physical!=1u)
        return 39;
    vat.status=0u;vat.calls=0u;
    if(port_v37_sysqio_device_access(PORT_DQ_READLBLK,
          PORT_V37_DEVCHAR_FOD|PORT_V37_DEVCHAR_MNT|PORT_V37_DEVCHAR_FOR,
          0u,0u,0x2345u,0x11223344u,0x4000u,check_volume,&vat,&physical,
          &access_status)!=PORT_V37_SYSQIO_DEVICE_NOPRIV||vat.calls!=1u||
       vat.kind!=PORT_V37_SYSQIO_ACCESS_LOGICAL||physical!=1u)
        return 40;
    vat.status=PORT_SYS85_NORMAL;vat.calls=0u;
    if(port_v37_sysqio_device_access(PORT_DQ_READLBLK,
          PORT_V37_DEVCHAR_FOD|PORT_V37_DEVCHAR_MNT,0u,0u,0u,0u,0u,
          check_volume,&vat,&physical,&access_status)!=
          PORT_V37_SYSQIO_DEVICE_NOPRIV||vat.calls||access_status!=PORT_SYS85_NOPRIV)
        return 41;
    if(port_v37_sysqio_device_access(PORT_DQ_READLBLK,
          PORT_V37_DEVCHAR_SPL,0u,1u,0u,0u,0u,check_volume,&vat,&physical,
          &access_status)!=PORT_V37_SYSQIO_DEVICE_OK||vat.calls||physical)
        return 42;
    if(port_v37_sysqio_device_access(PORT_DQ_READVBLK,
          PORT_V37_DEVCHAR_SPL,0u,0u,0u,0u,0u,check_volume,&vat,&physical,
          &access_status)!=PORT_V37_SYSQIO_DEVICE_OK||vat.calls||physical)
        return 43;
    if(port_v37_sysqio_device_access(PORT_DQ_READPBLK,
          PORT_V37_DEVCHAR_SPL,1u,0u,0u,0u,0u,check_volume,&vat,&physical,
          &access_status)!=PORT_V37_SYSQIO_DEVICE_OK||vat.calls||!physical)
        return 44;
    puts("SYS$QIO IRP, AST, device-state and VMS privilege checks PASS");
    return 0;
}
