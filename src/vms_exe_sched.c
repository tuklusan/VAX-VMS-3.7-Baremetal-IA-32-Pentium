#include "vms_exe_sched.h"
#include <stddef.h>
void port_exe_sched_init(struct port_exe_scheduler *s,uint8_t pcb_type) {
    if(!s)return;
    for(unsigned i=0;i<32;i++)s->head[i]=s->tail[i]=NULL;
    s->ready_mask=0;
    s->current=NULL;
    s->current_priority=32u;
    s->pcb_type=pcb_type;
}
uint32_t port_exe_sched_enqueue(struct port_exe_scheduler *s,
                               struct port_exe_sched_process *p) {
    if(!s || !p || p->type!=s->pcb_type ||
       p->current_priority>31u || p->state==PORT_EXE_STATE_READY)
        return PORT_SS_BADPARAM;
    uint8_t pri=p->current_priority;
    p->next=NULL;
    p->previous=s->tail[pri];
    if(s->tail[pri])s->tail[pri]->next=p;
    else s->head[pri]=p;
    s->tail[pri]=p;
    s->ready_mask|=UINT32_C(1)<<pri;
    p->state=PORT_EXE_STATE_READY;
    return PORT_SS_NORMAL;
}
uint32_t port_exe_sched_resched(struct port_exe_scheduler *s,
                               const struct port_exe_sched_ops *ops) {
    if(!s || !ops || !ops->save_context)return PORT_SS_BADPARAM;
    struct port_exe_sched_process *p=s->current;
    if(!p || p->state!=PORT_EXE_STATE_CURRENT)return PORT_SS_BADPARAM;
    ops->save_context(ops->context,p);
    s->current=NULL;
    uint32_t status=port_exe_sched_enqueue(s,p);
    if(!(status&1u))return status;
    return port_exe_sched_select(s,ops);
}
uint32_t port_exe_sched_select(struct port_exe_scheduler *s,
                              const struct port_exe_sched_ops *ops) {
    if(!s || !ops || !ops->load_context || !ops->idle_until_interrupt ||
       !ops->fatal_bad_queue)return PORT_SS_BADPARAM;
    while(!s->ready_mask) {
        s->current_priority=32u;
        ops->idle_until_interrupt(ops->context);
    }
    unsigned pri=0;
    while(pri<32u && !(s->ready_mask&(UINT32_C(1)<<pri)))++pri;
    if(pri>=32u || !s->head[pri]) {
        ops->fatal_bad_queue(ops->context);
        return PORT_SS_BADQUEUEHDR;
    }
    struct port_exe_sched_process *p=s->head[pri];
    s->head[pri]=p->next;
    if(s->head[pri])s->head[pri]->previous=NULL;
    else {
        s->tail[pri]=NULL;
        s->ready_mask&=~(UINT32_C(1)<<pri);
    }
    p->next=p->previous=NULL;
    if(p->type!=s->pcb_type) {
        ops->fatal_bad_queue(ops->context);
        return PORT_SS_BADQUEUEHDR;
    }
    p->state=PORT_EXE_STATE_CURRENT;
    s->current=p;
    /* Priorities 16..31 are time-sharing; float by one toward base. */
    if(p->base_priority!=p->current_priority &&
       (p->current_priority&16u))++p->current_priority;
    s->current_priority=p->current_priority;
    ops->load_context(ops->context,p);
    return PORT_SS_NORMAL;
}
