#include "vms_exe_getpti.h"
#include <stddef.h>
/* The only information code in SYSGETPTI.MAR is bit zero, demand zero. */
static int demand_zero(const struct port_exe_pti_page *p) {
    if(p->type==PORT_PTI_TRANSITION)return p->frame_or_section==0u;
    if(p->type==PORT_PTI_SECTION)return p->section_demand_zero!=0u;
    if(p->type==PORT_PTI_GLOBAL)return !p->global_valid &&
        p->global_type==PORT_PTI_SECTION && p->section_demand_zero;
    return 0;
}
uint32_t port_exe_getpti(uint8_t previous_mode,
                         const struct port_exe_pti_request *request,
                         const struct port_exe_pti_ops *ops) {
    if(!request || !ops || !ops->get_page)return PORT_SS_BADPARAM;
    uint32_t range[2];
    if(!request->input_range ||
       !port_exe_read(ops->user,request->input_range,range,8u))
        return PORT_SS_ACCVIO;
    uint8_t mode=(uint8_t)(request->access_mode&3u);
    if(mode<(previous_mode&3u))mode=previous_mode&3u;
    /* MMG$CREDEL rejects range crossings into VAX system space. */
    if((range[0]>>31)!=(range[1]>>31) || (range[0]>>31))
        return PORT_SS_NOPRIV;
    /* The source operates on 512-byte VAX virtual pages, inclusive. */
    uint32_t start=range[0]&~511u,finish=range[1]&~511u;
    int reverse=(range[0]>range[1]);
    if(range[0]==range[1] && (range[0]&UINT32_C(0x40000000)))reverse=1;
    uint32_t pages=reverse?(start-finish)/512u+1u:
                           (finish-start)/512u+1u;
    uint32_t required=request->minimum_pages?request->minimum_pages:1u;
    uint32_t count=0,first=0,last=0;
    uint8_t found=0;
    uint32_t result=PORT_SS_NORMAL;
    for(uint32_t i=0;i<pages;i++) {
        uint32_t va=reverse?start-i*512u:start+i*512u;
        struct port_exe_pti_page page={0};
        result=ops->get_page(ops->context,va,mode,&page);
        if(!(result&1u))break;
        int match=(request->mask&PORT_PTI_DEMAND_ZERO) && demand_zero(&page);
        if(match) {
            if(!count)first=va;
            last=va;
            ++count;
        } else if(count) {
            if(count>=required) {found=1;break;}
            count=0;
        }
    }
    if(!found && count>=required)found=1;
    if(found && request->return_range) {
        /* Output follows scan direction. Backward end is page-aligned. */
        uint32_t answer[2]={first,last};
        if(reverse)answer[0]|=511u;
        else answer[1]|=511u;
        if(!port_exe_probe_write(ops->user,request->return_range,8u) ||
           !port_exe_write(ops->user,request->return_range,answer,8u))
            return PORT_SS_ACCVIO;
    }
    /* The original suppresses a return range when no matching run exists. */
    return result;
}
