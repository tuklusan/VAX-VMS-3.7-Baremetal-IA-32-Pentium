#include "vms_exe_dclexh.h"
#include <stddef.h>
static uint32_t choose(struct port_exe_exit_handlers *h,uint8_t mode,
                       const struct port_exe_dclexh_ops *ops) {
    if(!h || !ops)return PORT_SS_BADPARAM;
    if(mode==0u || mode>3u) {
        if(ops->bugcheck_invalid_service)
            ops->bugcheck_invalid_service(ops->context);
        return PORT_SS_IVSSRQ;
    }
    return PORT_SS_NORMAL;
}
uint32_t port_exe_dclexh(struct port_exe_exit_handlers *h,uint8_t mode,
                         uint32_t block,const struct port_exe_dclexh_ops *ops) {
    uint32_t st=choose(h,mode,ops);
    if(!(st&1u))return st;
    if(!block)return PORT_SS_NOHANDLER;
    if(!port_exe_probe_write(ops->memory,block,4u))return PORT_SS_ACCVIO;
    uint32_t old=h->head[mode];
    if(!port_exe_write(ops->memory,block,&old,4u))return PORT_SS_ACCVIO;
    h->head[mode]=block;
    return PORT_SS_NORMAL;
}
uint32_t port_exe_canexh(struct port_exe_exit_handlers *h,uint8_t mode,
                         uint32_t block,const struct port_exe_dclexh_ops *ops) {
    uint32_t st=choose(h,mode,ops);
    if(!(st&1u))return st;
    if(!block){h->head[mode]=0;return PORT_SS_NORMAL;}
    uint32_t prev=0,cur=h->head[mode];
    /* A corrupt cyclic list must not trap the system in the service. */
    for(uint32_t count=0;cur && count<65536u;count++){
        if(!port_exe_probe_read(ops->memory,cur,4u))return PORT_SS_ACCVIO;
        uint32_t next=0;
        if(!port_exe_read(ops->memory,cur,&next,4u))return PORT_SS_ACCVIO;
        if(cur==block){
            if(prev) {
                if(!port_exe_probe_write(ops->memory,prev,4u) ||
                   !port_exe_write(ops->memory,prev,&next,4u))return PORT_SS_ACCVIO;
            }else h->head[mode]=next;
            return PORT_SS_NORMAL;
        }
        prev=cur;cur=next;
    }
    return PORT_SS_NOHANDLER;
}
