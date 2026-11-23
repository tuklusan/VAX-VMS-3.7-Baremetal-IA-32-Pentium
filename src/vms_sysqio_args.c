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

/* SYSQIOREQ.MAR 253-283. CCB$B_AMOD is MAXACMODE+1 as stored by SYS$ASSIGN;
 * the strict comparison therefore admits a request from the assigning mode
 * while rejecting access from a less privileged mode. */
uint32_t port_v37_sysqio_preflight(const struct port_v37_sysqio_args *args,
    uint8_t caller_mode,const struct port_v37_sysqio_device *device,
    struct port_v37_sysqio_dispatch *dispatch){
    if(!args||!device||!dispatch||caller_mode>3u||
       !device->fdt||!device->fdt_bytes||
       device->ccb_access_limit==0u||device->ccb_access_limit>4u||
       device->online>1u)
        return PORT_V37_SYSQIO_PRECHECK_BAD;
    if(caller_mode>=device->ccb_access_limit)
        return PORT_V37_SYSQIO_PRECHECK_NOPRIV;
    struct port_dq_fdt_decision fdt={0};
    if(port_dq_fdt_classify(device->fdt,device->fdt_bytes,args->function,
                            &fdt)!=PORT_DQ_FDT_OK)
        return PORT_V37_SYSQIO_PRECHECK_ILLFUNC;
    if(!device->online&&fdt.number!=PORT_DQ_DEACCESS&&
       fdt.number!=PORT_DQ_ACPCONTROL)
        return PORT_V37_SYSQIO_PRECHECK_OFFLINE;
    const struct port_v37_sysqio_dispatch out={
        .full_function=args->function,.function_code=fdt.number,
        .buffered=fdt.buffered,.action_class=fdt.action_class};
    *dispatch=out;
    return PORT_V37_SYSQIO_PRECHECK_OK;
}
