#include "vms_exe_setprt.h"
#include <stddef.h>
static int can_write(uint8_t p){return p!=0u && (p&3u)!=3u;}
static uint32_t adjust_one(struct port_exe_page_state *p,uint8_t mode,
                           uint8_t desired,int32_t *quota,
                           const struct port_exe_setprt_ops *ops){
    if(!p->exists)return PORT_SS_ACCVIO;
    if(mode>p->owner_mode)return PORT_SS_PAGOWNVIO;
    uint8_t old=p->protection;
    int copy=(can_write(desired) && !can_write(old));
    switch(p->type){
      case PORT_EXE_PTE_TRANSITION:
        if(p->pfn || !can_write(desired)){
            if(!ops->fault_page)return PORT_SS_ACCVIO;
            uint32_t st=ops->fault_page(ops->context,p->virtual_address);
            if(!(st&1u))return st;
            st=ops->get_page(ops->context,p->virtual_address,p);
            if(!(st&1u))return st;
            if(!p->exists || p->type==PORT_EXE_PTE_TRANSITION)
                return PORT_SS_ACCVIO;
            return adjust_one(p,mode,desired,quota,ops);
        }
        break;
      case PORT_EXE_PTE_GLOBAL:
        if(!p->global_crf)return PORT_SS_NOPRIV;
        break;
      case PORT_EXE_PTE_SECTION:
        if(copy && !p->section_writable){
            if(!quota || *quota<=0)return PORT_SS_EXQUOTA_M;
            --*quota;
            p->copy_on_reference=1;
        }
        break;
      case PORT_EXE_PTE_VALID:
        if(p->shared_memory || p->window_mapping || !p->process_page)
            return PORT_SS_NOPRIV;
        if(copy && p->backed_section && !p->backing_writable){
            if(!quota || *quota<=0)return PORT_SS_EXQUOTA_M;
            --*quota;
            if(!ops->decrement_section)return PORT_SS_BADPARAM;
            uint32_t st=ops->decrement_section(ops->context,p->pfn);
            if(!(st&1u)){++*quota;return st;}
            p->backed_section=0;
            p->modified=1;
        }
        break;
      case PORT_EXE_PTE_PAGEFILE:
        break;
      default:return PORT_SS_BADPARAM;
    }
    p->protection=desired;
    if(!ops->set_page)return PORT_SS_BADPARAM;
    uint32_t st=ops->set_page(ops->context,p);
    if(!(st&1u))return st;
    if(ops->invalidate_tlb)ops->invalidate_tlb(ops->context,p->virtual_address);
    return PORT_SS_NORMAL;
}
uint32_t port_exe_setprt(const struct port_exe_setprt_request *req,
                         uint8_t previous_mode,int32_t *quota,
                         const struct port_exe_setprt_ops *ops) {
    if(!req || !ops || !ops->memory || !ops->get_page || !ops->set_page ||
       !ops->lock_pages || !ops->unlock_pages)return PORT_SS_BADPARAM;
    uint32_t desired=req->protection&255u;
    if(desired==1u || desired>15u)return PORT_SS_IVPROTECT;
    if(!desired)desired=4u; /* Source raises NO ACCESS to kernel read. */
    uint32_t range[2];
    if(!req->input_range || !port_exe_read(ops->memory,req->input_range,range,8u))
        return PORT_SS_ACCVIO;
    uint8_t mode=(uint8_t)(req->requested_mode&3u);
    if(mode<previous_mode)mode=previous_mode;
    uint32_t start=range[0]&~511u,end=range[1]&~511u;
    uint32_t result[2]={UINT32_MAX,UINT32_MAX};
    uint8_t oldprot=0;
    uint32_t status=PORT_SS_NORMAL;
    int step=start<=end?512:-512;
    ops->lock_pages(ops->context);
    for(uint32_t at=start;;at=(uint32_t)((int64_t)at+step)){
        if(at>=0x80000000u || (int32_t)at<0){status=PORT_SS_NOPRIV;break;}
        struct port_exe_page_state p={0};
        status=ops->get_page(ops->context,at,&p);
        if(!(status&1u))break;
        oldprot=p.protection;
        status=adjust_one(&p,mode,(uint8_t)desired,quota,ops);
        if(!(status&1u))break;
        if(result[0]==UINT32_MAX)result[0]=at;
        result[1]=at+511u;
        if(at==end)break;
        if((step>0 && at>end-512u) || (step<0 && at<end+512u))break;
    }
    ops->unlock_pages(ops->context);
    if(req->return_range &&
       (!port_exe_probe_write(ops->memory,req->return_range,8u) ||
        !port_exe_write(ops->memory,req->return_range,result,8u)))
        return PORT_SS_ACCVIO;
    if(req->old_protection &&
       (!port_exe_probe_write(ops->memory,req->old_protection,1u) ||
        !port_exe_write(ops->memory,req->old_protection,&oldprot,1u)))
        return PORT_SS_ACCVIO;
    return status;
}
