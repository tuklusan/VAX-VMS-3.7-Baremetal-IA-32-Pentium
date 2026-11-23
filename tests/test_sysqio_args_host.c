#include "vms_sysqio_args.h"
#include <stdio.h>
#include <string.h>

static void put32(uint8_t *p,uint32_t v){
    p[0]=(uint8_t)v;p[1]=(uint8_t)(v>>8);
    p[2]=(uint8_t)(v>>16);p[3]=(uint8_t)(v>>24);
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
    puts("SYS$QIO AP argument layout and fail-atomic decode PASS");
    return 0;
}
