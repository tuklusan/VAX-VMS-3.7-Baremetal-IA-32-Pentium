#include "vms_sysqio_args.h"
#include "vms_sysqio_ef.h"
#include "vms_sys85.h"
#include <stdio.h>
#include <string.h>

static void put32(uint8_t *p,uint32_t v){
    p[0]=(uint8_t)v;p[1]=(uint8_t)(v>>8);
    p[2]=(uint8_t)(v>>16);p[3]=(uint8_t)(v>>24);
}
struct global_ef_test { uint8_t seen;uint32_t status; };
static uint32_t clear_global(void *opaque,uint8_t efn){
    struct global_ef_test *t=opaque;t->seen=efn;return t->status;
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
    struct port_v37_sysqio_device device={&fdt,sizeof fdt,4u,1u};
    struct port_v37_sysqio_dispatch action={0};
    struct port_v37_sysqio_args request={.function=0x80ccu};
    if(port_v37_sysqio_preflight(&request,3u,&device,&action)!=
       PORT_V37_SYSQIO_PRECHECK_OK||action.full_function!=0x80ccu||
       action.function_code!=PORT_DQ_READPBLK)return 7;
    request.function=PORT_DQ_READLBLK;
    device.ccb_access_limit=3u;
    memset(&action,0x5a,sizeof action);
    struct port_v37_sysqio_dispatch saved=action;
    if(port_v37_sysqio_preflight(&request,3u,&device,&action)!=
       PORT_V37_SYSQIO_PRECHECK_NOPRIV||memcmp(&action,&saved,sizeof action))
        return 8;
    device.ccb_access_limit=4u;device.online=0u;
    if(port_v37_sysqio_preflight(&request,3u,&device,&action)!=
       PORT_V37_SYSQIO_PRECHECK_OFFLINE)return 9;
    request.function=PORT_DQ_DEACCESS;
    if(port_v37_sysqio_preflight(&request,3u,&device,&action)!=
       PORT_V37_SYSQIO_PRECHECK_OK)return 10;
    request.function=63u;
    if(port_v37_sysqio_preflight(&request,3u,&device,&action)!=
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
    puts("SYS$QIO EFN/AP/channel mode/FDT preflight cases PASS");
    return 0;
}
