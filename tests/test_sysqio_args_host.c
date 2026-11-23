#include "vms_sysqio_args.h"
#include "vms_qio_post.h"
#include "vms_sysqio_ef.h"
#include "vms_sysqio_iosb.h"
#include "vms_sys85.h"
#include "vms_exe_schevt.h"
#include "vms_v37_dyn_tags.h"
#include <stdio.h>
#include <string.h>

static void put32(uint8_t *p,uint32_t v){
    p[0]=(uint8_t)v;p[1]=(uint8_t)(v>>8);
    p[2]=(uint8_t)(v>>16);p[3]=(uint8_t)(v>>24);
}
static uint16_t get16(const uint8_t *p){
    return (uint16_t)((uint16_t)p[0]|((uint16_t)p[1]<<8));
}
static uint32_t get32(const uint8_t *p){
    return (uint32_t)p[0]|((uint32_t)p[1]<<8)|
           ((uint32_t)p[2]<<16)|((uint32_t)p[3]<<24);
}
struct global_ef_test { uint8_t seen;uint32_t status; };
static uint32_t clear_global(void *opaque,uint8_t efn){
    struct global_ef_test *t=opaque;t->seen=efn;return t->status;
}
struct iosb_memory_test {
    uint32_t base;
    uint8_t bytes[32];
    uint32_t probes,clears;
    int deny_probe,deny_clear;
};
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
    uint32_t quota_calls;uint8_t quota_buffered;int deny_channel,grant_quota;
};
static int lookup_channel(void *opaque,uint32_t pid,uint32_t channel,
    struct port_v37_sysqio_resolved_channel *out){
    struct begin_rig *r=opaque;r->lookups++;
    if(r->deny_channel||pid!=r->expected_pid||channel!=r->expected_channel)
        return -1;
    out->device=r->device;out->ucb=r->ucb;
    out->ccb_access_limit=r->ccb_access_limit?r->ccb_access_limit:4u;
    return 0;
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
    const uint32_t values[12]={3u,0x12340002u,0xffff0031u,0x1000u,
        0x2000u,0xaabbccddu,1u,2u,3u,4u,5u,6u};
    for(unsigned i=0;i<12u;i++)put32(ap+4u+4u*i,values[i]);
    struct port_v37_sysqio_args out;
    memset(&out,0xa5,sizeof out);
    if(port_v37_sysqio_decode_args(ap,sizeof ap,&out)!=PORT_V37_SYSQIO_ARGS_OK||
       out.event_flag!=3u||out.channel!=0x12340002u||out.function!=0x31u||
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
    struct port_v37_sysqio_device device={&fdt,sizeof fdt,1u};
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
        .expected_pid=7u,.expected_channel=0x12340002u};
    struct port_v37_sysqio_process proc={.pid=7u,.access_mode=3u,
        .local_event_flags={UINT32_MAX,UINT32_MAX}};
    const struct port_v37_sysqio_begin_ops begin_ops={
        .context=&rig,.lookup_channel=lookup_channel,.post_event=post_event,
        .single_io_quota=single_quota,.user_memory=memops};
    struct port_v37_sysqio_prepared prepared={0};
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
    rig.deny_channel=1;rig.posts=0u;rig.lookups=0u;
    memset(mem.bytes,0xa5,sizeof mem.bytes);mem.probes=mem.clears=0u;
    struct port_v37_sysqio_prepared untouched;
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
        .window_va=0u,.channel_index=2u,.base_priority=8u};
    if(port_v37_sysqio_build_irp(&prepared,&proc,&ispec,irp,sizeof irp)!=
         PORT_V37_SYSQIO_IRP_BUILD_OK||
       get16(irp+8u)!=PORT_V37_SYSQIO_IRP_STANDARD_BYTES||
       irp[10u]!=PORT_V37_DYN_IRP||irp[11u]!=0x43u||
       get32(irp+12u)!=7u||get32(irp+16u)!=0x2000u||
       get32(irp+20u)!=0xaabbccddu||get32(irp+24u)!=0u||
       get32(irp+28u)!=0x2000u||get16(irp+32u)!=0x31u||
       irp[34u]!=3u||irp[35u]!=8u||get32(irp+36u)!=0x1000u||
       get16(irp+40u)!=2u||get16(irp+42u)!=0u||
       get32(irp+44u)!=0u||get16(irp+48u)!=0u||
       get32(irp+52u)!=0u||get32(irp+PORT_V37_IRP_ARB)!=0x3000u||
       irp[PORT_V37_SYSQIO_IRP_STANDARD_BYTES]!=0xa5u){
        return 28;
    }
    uint32_t ast_status=0u;
    if(proc.ast_count!=-1||port_v37_sysqio_check_ast_quota(&prepared,&proc,
          &ast_status)!=PORT_V37_SYSQIO_AST_EXQUOTA||
       ast_status!=PORT_SS_EXQUOTA)
        return 36;
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
    puts("SYS$QIO begin, source IRP layout and virtual-write normalization PASS");
    return 0;
}
