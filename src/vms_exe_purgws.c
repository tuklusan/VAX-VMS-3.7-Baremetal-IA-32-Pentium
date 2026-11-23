#include "vms_exe_purgws.h"
#include <stddef.h>
uint32_t port_exe_purgws_scan(uint32_t start,uint32_t end,uint8_t mode,
                              uint8_t only_globals,
                              const struct port_exe_purgws_ops *ops,
                              unsigned *removed){
    if(removed)*removed=0;
    if(!ops || !ops->entries || !ops->free_entry || !ops->lock_ws ||
       !ops->unlock_ws)return PORT_SS_BADPARAM;
    uint32_t low=(start<end?start:end)&~511u;
    uint32_t high=(start>end?start:end)&~511u;
    if(high>=0x80000000u)return PORT_SS_NOPRIV;
    unsigned freed=0;
    uint32_t status=PORT_SS_NORMAL;
    ops->lock_ws(ops->context);
    unsigned begin=only_globals?0u:ops->locked_count;
    if(begin>ops->entry_count)begin=ops->entry_count;
    for(unsigned i=begin;i<ops->entry_count;i++){
        struct port_exe_ws_entry *e=&ops->entries[i];
        if(!e->valid || (only_globals && !e->global_page) ||
           (!only_globals && (e->ws_locked || e->pfn_locked)))continue;
        uint32_t at=e->virtual_address&~511u;
        if(at<low || at>high)continue;
        if(mode>e->owner_mode)continue;
        if(e->ws_locked || e->pfn_locked){
            if(!only_globals || !ops->unlock_global)continue;
            status=ops->unlock_global(ops->context,e);
            if(!(status&1u))break;
        }
        status=ops->free_entry(ops->context,e);
        if(!(status&1u)) {
            /* $PURGWS keeps this entry if the page-file pool is full. */
            if(!only_globals){status=PORT_SS_NORMAL;continue;}
            break;
        }
        e->valid=0;
        ++freed;
    }
    ops->unlock_ws(ops->context);
    if(removed)*removed=freed;
    return status;
}
uint32_t port_exe_purgws(uint32_t input_range,uint8_t previous_mode,
                         const struct port_exe_purgws_ops *ops,unsigned *removed) {
    if(!ops || !ops->memory || !input_range)return PORT_SS_ACCVIO;
    uint32_t range[2];
    if(!port_exe_read(ops->memory,input_range,range,8u))return PORT_SS_ACCVIO;
    return port_exe_purgws_scan(range[0],range[1],previous_mode,
                                0u,ops,removed);
}
