#include "vms_dcl_exit.h"
#include "vms_bintim.h"

int port_dcl_exit(struct port_dcl_process *p,const char *expr,
                  const struct port_dcl_ops *ops,void *ctx,uint32_t *result) {
    if(!p||!ops||!result)return PORT_DCL_EXIT_PARSE_ERROR;
    uint32_t status=p->last_status|PORT_DCL_INHIB_MSG;
    while(expr&&(*expr==' '||*expr=='\t'))expr++;
    if(expr&&*expr){
        uint32_t evaluated=0;
        if(!ops->binary_expression){*result=0;return PORT_DCL_EXIT_PARSE_ERROR;}
        uint32_t rc=ops->binary_expression(ctx,expr,&evaluated);
        if(!(rc&1u)){
            *result=rc;
            return PORT_DCL_EXIT_PARSE_ERROR;
        }
        status=evaluated;
    }
    p->last_status=status;
    if(p->indirect_depth){
        if(ops->unstack)ops->unstack(ctx);
        p->indirect_depth--;
        p->command_flags|=PORT_DCL_COMMAND;
        *result=status;
        return PORT_DCL_EXIT_RETURN;
    }
    if(p->interactive){
        p->command_flags|=PORT_DCL_COMMAND;
        if(ops->rundown)ops->rundown(ctx);
        *result=p->last_status;
        return PORT_DCL_EXIT_RETURN;
    }
    p->exiting=1;
    if(ops->abort_process)ops->abort_process(ctx);
    *result=status;
    return PORT_DCL_EXIT_LOGOUT;
}

int port_dcl_wait_delta(const char *time,uint64_t *ticks100ns) {
    if(!time||!ticks100ns)return 0;
    size_t length=0u;
    while(time[length]&&length<253u)++length;
    if(time[length])return 0;
    char input[255]={'0',' '};
    for(size_t i=0u;i<length;i++)input[i+2u]=time[i];
    uint64_t binary=0u;
    if(port_vms_bintim(input,length+2u,0u,&binary)!=PORT_BINTIM_NORMAL)
        return 0;
    *ticks100ns=UINT64_C(0)-binary;
    return 1;
}
int port_dcl_wait(const char *spec,const struct port_dcl_ops *ops,void *ctx) {
    if(!spec||!ops||!ops->cancel_timers||!ops->set_timer||
       !ops->wait_flag)return 0;
    if(!ops->cancel_timers(ctx))return 0;
    uint64_t ticks=0;
    int ok=ops->binary_delta?
           ops->binary_delta(ctx,spec,&ticks):port_dcl_wait_delta(spec,&ticks);
    if(!ok)return 0;
    if(!ops->set_timer(ctx,PORT_DCL_WAIT_EFN,ticks))return 0;
    return ops->wait_flag(ctx,PORT_DCL_WAIT_EFN);
}
