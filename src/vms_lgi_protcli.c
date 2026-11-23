#include "vms_lgi_protcli.h"
#include <stddef.h>
uint32_t port_lgi_protect_cli(const uint32_t image_range[2],
                              const struct port_lgi_protcli_ops *ops){
    if(!image_range||!ops||!ops->lock_routine||!ops->get_page||!ops->set_page||
       !ops->user_writable||!ops->invalidate||!ops->lock_tables||
       !ops->unlock_tables)return PORT_SS_BADPARAM;
    uint32_t st=ops->lock_routine(ops->context);
    if(!(st&1u))return st;
    uint32_t low=image_range[0],high=image_range[1];
    if(low>high){uint32_t temp=low;low=high;high=temp;}
    low&=~511u;high&=~511u;
    ops->lock_tables(ops->context);
    uint32_t current=high;
    for(;;){
        struct port_exe_page_state p={0};
        st=ops->get_page(ops->context,current,&p);
        if(!(st&1u))break;
        if(p.owner_mode==3u){
            p.owner_mode=2u;
            if(ops->user_writable(ops->context,current))p.protection=12u;
            st=ops->set_page(ops->context,&p);
            if(!(st&1u))break;
            if(p.type==PORT_EXE_PTE_VALID)ops->invalidate(ops->context,current);
        }
        if(current==low){st=PORT_SS_NORMAL;break;}
        current-=512u;
    }
    ops->unlock_tables(ops->context);
    return st;
}
uint32_t port_lgi_execute_image(const struct port_lgi_execute_ops *ops){
    if(!ops||!ops->rundown_user||!ops->activate_image)return PORT_SS_BADPARAM;
    uint32_t st=ops->rundown_user(ops->context);
    if(!(st&1u))return st;
    st=ops->activate_image(ops->context);
    return (st&1u)? PORT_SS_BADPARAM:st;
}
