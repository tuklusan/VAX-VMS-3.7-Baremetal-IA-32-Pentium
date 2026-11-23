#include "vms_dcl_if.h"
int port_dcl_if(const char *expression,const char *then_keyword,
                const char *then_command,uint32_t *flags,
                const struct port_dcl_if_ops *ops,void *ctx,
                uint32_t *completion){
    if(!flags||!ops||!ops->binary_expression||!completion||
       !(ops->normal_status&1u)|| (ops->invalid_then_status&1u))return -1;
    *flags|=PORT_DCL_IF_NOSTAT;
    uint32_t value=0;
    uint32_t result=ops->binary_expression(ctx,expression,&value);
    if(!(result&1u)){
        *completion=result;
        return 0;
    }
    if(!then_keyword||then_keyword[0]!='T'||then_keyword[1]!='H'||
       then_keyword[2]!='E'||then_keyword[3]!='N'||then_keyword[4]){
        if(ops->mark)ops->mark(ctx);
        *completion=ops->invalid_then_status;
        return 0;
    }
    *completion=ops->normal_status;
    if(!(value&1u))return 1;
    if(!ops->command_start||!then_command)return -1;
    return ops->command_start(ctx,then_command)?PORT_DCL_IF_RUN_THEN:0;
}
