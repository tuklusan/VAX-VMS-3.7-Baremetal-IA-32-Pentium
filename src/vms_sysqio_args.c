#include "vms_sysqio_args.h"

static uint32_t load32(const uint8_t *p){
    return (uint32_t)p[0]|((uint32_t)p[1]<<8)|
           ((uint32_t)p[2]<<16)|((uint32_t)p[3]<<24);
}

/* SYS/SRC/SYSQIOREQ.MAR 239-272 and 286-290. The source entry reads the
 * argument count at AP+0 and uses fixed longword slots through P6 at AP+48;
 * FUNC is a word value and is zero-extended by the service. */
uint32_t port_v37_sysqio_decode_args(const void *argument_list,
    size_t accessible_bytes,struct port_v37_sysqio_args *decoded){
    if(!argument_list||!decoded||((uintptr_t)argument_list&3u)||
       accessible_bytes<PORT_V37_SYSQIO_LIST_BYTES)
        return PORT_V37_SYSQIO_ARGS_BAD;
    const uint8_t *const ap=argument_list;
    if(load32(ap)<PORT_V37_SYSQIO_REQUIRED_ARGS)
        return PORT_V37_SYSQIO_ARGS_BAD;
    struct port_v37_sysqio_args out={0};
    out.event_flag=load32(ap+4u);
    out.channel=load32(ap+8u);
    out.function=(uint16_t)load32(ap+12u);
    out.iosb=load32(ap+16u);
    out.ast_address=load32(ap+20u);
    out.ast_parameter=load32(ap+24u);
    for(unsigned i=0;i<6u;i++)out.parameter[i]=load32(ap+28u+4u*i);
    *decoded=out;
    return PORT_V37_SYSQIO_ARGS_OK;
}
