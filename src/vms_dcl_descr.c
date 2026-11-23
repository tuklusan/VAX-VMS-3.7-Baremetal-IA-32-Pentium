#include "vms_dcl_descr.h"
int port_dcl_getdval(struct port_dcl_descr_state *state,
                      struct port_dcl_descr_result *result){
    if(!state||!result||!state->command||!state->next||!state->end||
       state->next>=state->end)return 0;
    uint32_t packed=*state->next;
    unsigned offset=(packed>>8)&0xfffu;
    if(offset>state->length)return 0;
    state->next++;
    result->value=packed&0xffu;
    result->item=state->command+offset;
    result->flags=(packed>>20)&15u;
    result->term=(packed>>24)&15u;
    result->type=(packed>>28)&15u;
    return 1;
}
