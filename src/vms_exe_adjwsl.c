#include "vms_exe_adjwsl.h"
#include <stddef.h>
static uint32_t minimum(uint32_t a,uint32_t b){return a<b?a:b;}
void port_exe_extra_dyn_ws(struct port_exe_working_set *w){
    if(!w)return;
    uint32_t unlocked=w->pt_max>w->pt_locked?
                      (uint32_t)(w->pt_max-w->pt_locked):0u;
    uint32_t needed=minimum(unlocked,w->fluid)+w->fluid;
    uint32_t locked=w->first_dynamic>w->first_index?
                    (uint32_t)(w->first_dynamic-w->first_index):0u;
    uint32_t total=needed+locked;
    w->extra_dynamic=w->size>total?(uint16_t)(w->size-total):0u;
}
void port_exe_wspchk(struct port_exe_working_set *w){
    if(!w)return;
    w->peak_check=(w->size>w->peak);
    w->image_peak_check=(w->image_counting && w->size>w->image_peak);
}
uint32_t port_exe_adjwsl(struct port_exe_working_set *w,int32_t pages,
                         uint32_t out,const struct port_exe_adjwsl_ops *ops){
    if(!w || !ops)return PORT_SS_BADPARAM;
    if(pages>=0){
        uint32_t avail_sys=w->system_max_ws>w->size?w->system_max_ws-w->size:0u;
        uint32_t max_process=(uint32_t)w->extent-w->first_index+1u;
        uint32_t avail_process=max_process>w->size?max_process-w->size:0u;
        uint32_t avail_phys=w->physical_pages>w->free_limit+w->size?
                            w->physical_pages-w->free_limit-w->size:0u;
        uint32_t allow=minimum(minimum((uint32_t)pages,avail_sys),
                               minimum(avail_process,avail_phys));
        uint32_t entries=(uint32_t)w->last_index-w->first_index+1u;
        uint32_t existing=entries>w->size?entries-w->size:0u;
        uint32_t free_slots=minimum(allow,existing);
        w->size=(uint16_t)(w->size+free_slots);
        allow-=free_slots;
        if(allow){
            if(!ops->expand_header)return PORT_SS_BADPARAM;
            uint32_t grown=0;
            uint32_t status=ops->expand_header(ops->context,allow,&grown);
            if(!(status&1u))return status;
            if(grown>allow)grown=allow;
            w->last_index=(uint16_t)(w->last_index+grown);
            w->size=(uint16_t)(w->size+grown);
        }
    }else{
        uint32_t requested=(uint32_t)(-(int64_t)pages);
        port_exe_extra_dyn_ws(w);
        uint32_t locked=(uint32_t)w->first_dynamic-w->first_index;
        uint32_t minimum_ws=w->min_ws+locked+1u;
        uint32_t may_reduce=w->size>minimum_ws?w->size-minimum_ws:0u;
        uint32_t shrink=minimum(requested,minimum(may_reduce,w->extra_dynamic));
        uint32_t present=w->used_process+w->used_global;
        uint32_t empty=w->size>present?w->size-present:0u;
        uint32_t easy=minimum(empty,shrink);
        w->size=(uint16_t)(w->size-easy);
        shrink-=easy;
        while(shrink){
            if(!ops->free_page)return PORT_SS_BADPARAM;
            uint32_t status=ops->free_page(ops->context);
            if(!(status&1u)){
                if(!ops->wait_resource)return status;
                uint32_t wait=ops->wait_resource(ops->context,status);
                if(!(wait&1u))return wait;
                continue;
            }
            --w->size;
            --shrink;
        }
    }
    port_exe_extra_dyn_ws(w);
    port_exe_wspchk(w);
    if(out && (!port_exe_probe_write(ops->memory,out,4u) ||
               !port_exe_write(ops->memory,out,&(uint32_t){w->size},4u)))
        return PORT_SS_ACCVIO;
    return PORT_SS_NORMAL;
}
