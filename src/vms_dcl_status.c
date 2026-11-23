#include "vms_dcl_status.h"

static void hex8(char out[8],uint32_t value){
    static const char digits[]="0123456789ABCDEF";
    for(unsigned i=0;i<8u;i++)out[i]=digits[(value>>(28u-4u*i))&15u];
}

int port_dcl_set_status(struct port_dcl_status_process *p,
                        struct port_dcl_status_work *w,uint32_t status,
                        const struct port_dcl_status_ops *ops,void *ctx){
    if(!p||!w)return -1;
    if(p->flags&PORT_DCL_STATUS_ONEXIT){
        p->flags=(uint8_t)(p->flags&~PORT_DCL_STATUS_ONEXIT);
        return 0;
    }
    if((status&1u) && (w->flags&PORT_DCL_STATUS_NOSTAT))return 0;
    if(ops&&ops->disable_control_ast)ops->disable_control_ast(ctx);
    uint32_t converted=status;
    if((int32_t)status<0){
        converted=0u-status;
        converted=(converted|0xe000u)*4u;
    }
    p->severity=(uint8_t)(converted&7u);
    p->severity_text[0]=(char)('0'+p->severity);
    p->severity_text[1]=0;
    p->status_text[0]='%';p->status_text[1]='X';
    hex8(p->status_text+2,converted);
    p->status_text[10]=0;
    if(p->flags&PORT_DCL_STATUS_YLEVEL)return 0;
    p->last_status=converted;
    if(converted&1u)return 0;
    if(p->severity<p->on_level)return 0;
    if(ops&&ops->flush)ops->flush(ctx);
    const char *command=p->on_error;
    if(!command){
        command="EXIT";
        p->flags=(uint8_t)(p->flags|PORT_DCL_STATUS_ONEXIT);
    }
    unsigned i=0;
    while(command[i]){
        if(i>=sizeof(w->command)-1u)return -1;
        w->command[i]=command[i];i++;
    }
    w->command[i]=0;
    w->char_index=0;
    if(ops&&ops->on_reset)ops->on_reset(ctx);
    if(ops&&ops->command_start)ops->command_start(ctx);
    return PORT_DCL_STATUS_ON_COMMAND;
}
