#include "vms_dcl_exit.h"

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

static int parse_two(const char **p,uint64_t *number,unsigned limit) {
    unsigned digits=0;uint64_t value=0;
    while(**p>='0'&&**p<='9'){
        if(++digits>2)return 0;
        value=value*10u+(unsigned)(*(*p)++-'0');
    }
    if(!digits||value>limit)return 0;
    *number=value;return 1;
}
int port_dcl_wait_delta(const char *time,uint64_t *ticks100ns) {
    if(!time||!ticks100ns)return 0;
    while(*time==' '||*time=='\t')time++;
    /* The DCL source prepends a zero-day field before BINTIM. */
    uint64_t hours,minutes,seconds,frac=0;uint32_t scale=10000000u;
    if(!parse_two(&time,&hours,23u)||*time++!=':'||
       !parse_two(&time,&minutes,59u)||*time++!=':'||
       !parse_two(&time,&seconds,59u))return 0;
    if(*time=='.'){
        time++;unsigned digits=0;
        while(*time>='0'&&*time<='9'){
            if(++digits>7)return 0;
            scale/=10u;frac+=(uint64_t)(*time++-'0')*scale;
        }
        if(!digits)return 0;
    }
    while(*time==' '||*time=='\t')time++;
    if(*time)return 0;
    *ticks100ns=((hours*3600u+minutes*60u+seconds)*10000000u)+frac;
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
