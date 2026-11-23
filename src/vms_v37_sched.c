#include "vms_v37_sched.h"
#include "vms_v37_dyn_tags.h"

enum {
    SQFL=PORT_V37_PCB_SQFL,SQBL=PORT_V37_PCB_SQBL,
    PCBPRI=PORT_V37_PCB_PRI,PCBSTATE=PORT_V37_PCB_STATE,
    PCBPRIB=PORT_V37_PCB_PRIB,PCBSTS=PORT_V37_PCB_STS,
    Q_INCREMENT=0,Q_COUNT=1,Q_TYPE=10
};
static const uint8_t priority_increment[PORT_V37_SCH_INCREMENT_CLASSES]={
    0u,2u,3u,4u,6u};

static uint16_t get16(const uint8_t *p){
    return (uint16_t)p[0]|(uint16_t)((uint16_t)p[1]<<8);
}
static uint32_t get32(const uint8_t *p){
    return (uint32_t)p[0]|((uint32_t)p[1]<<8)|
        ((uint32_t)p[2]<<16)|((uint32_t)p[3]<<24);
}
static void put16(uint8_t *p,uint16_t x){
    p[0]=(uint8_t)x;p[1]=(uint8_t)(x>>8);
}
static void put32(uint8_t *p,uint32_t x){
    p[0]=(uint8_t)x;p[1]=(uint8_t)(x>>8);
    p[2]=(uint8_t)(x>>16);p[3]=(uint8_t)(x>>24);
}
static uint8_t *resolve(const struct port_v37_scheduler *s,uint32_t address,
                        size_t bytes){
    if(!address||!s->ops.resolve)return 0;
    return s->ops.resolve(s->ops.context,address,bytes);
}
static uint32_t head_address(const struct port_v37_scheduler *s,unsigned q){
    return s->ops.address(s->ops.context,&s->heads[q]);
}
static uint32_t set_summary(struct port_v37_scheduler *s,unsigned q,int set){
    if(q>=PORT_V37_SCH_QUEUES)return PORT_V37_SCHED_BADPARAM;
    uint32_t mask=1u<<(q&31u);
    if(set)s->summary[q>>5]|=mask;
    else s->summary[q>>5]&=~mask;
    return PORT_V37_SCHED_OK;
}
static int list_empty(const struct port_v37_scheduler *s,unsigned q){
    uint32_t h=head_address(s,q);
    return h&&s->heads[q].forward==h&&s->heads[q].backward==h;
}
static uint32_t list_insert_tail(struct port_v37_scheduler *s,unsigned q,
                                  uint32_t node){
    const uint32_t h=head_address(s,q);
    uint8_t *n=resolve(s,node,PORT_V37_PCB_BYTES);
    uint8_t *tail=resolve(s,s->heads[q].backward,sizeof(struct port_v37_sched_head));
    uint8_t *head=resolve(s,h,sizeof(struct port_v37_sched_head));
    if(!h||!n||!tail||!head)return PORT_V37_SCHED_UNRESOLVED;
    put32(n+SQFL,h);put32(n+SQBL,s->heads[q].backward);
    put32(tail+SQFL,node);put32(head+SQBL,node);
    if(s->heads[q].forward==h)put32(head+SQFL,node);
    return PORT_V37_SCHED_OK;
}
static uint32_t list_remove(struct port_v37_scheduler *s,uint32_t node,
                             uint8_t *pcb){
    const uint32_t next=get32(pcb+SQFL),previous=get32(pcb+SQBL);
    if(next==node&&previous==node)return PORT_V37_SCHED_OK;
    uint8_t *n=resolve(s,next,sizeof(struct port_v37_sched_head));
    uint8_t *p=resolve(s,previous,sizeof(struct port_v37_sched_head));
    if(!n||!p||get32(n+SQBL)!=node||get32(p+SQFL)!=node)
        return PORT_V37_SCHED_CORRUPT;
    put32(p+SQFL,next);put32(n+SQBL,previous);
    put32(pcb+SQFL,node);put32(pcb+SQBL,node);
    return PORT_V37_SCHED_OK;
}
static uint32_t list_contains(const struct port_v37_scheduler *s,unsigned q,
                              uint32_t node,int *found){
    const uint32_t h=head_address(s,q);
    uint32_t cursor=s->heads[q].forward,previous=h;
    uint32_t limit=8192u;
    *found=0;
    while(cursor!=h&&limit--){
        uint8_t *p=resolve(s,cursor,PORT_V37_PCB_BYTES);
        if(!p||get32(p+SQBL)!=previous)return PORT_V37_SCHED_CORRUPT;
        if(cursor==node)*found=1;
        previous=cursor;cursor=get32(p+SQFL);
    }
    if(cursor!=h||s->heads[q].backward!=previous)
        return PORT_V37_SCHED_CORRUPT;
    return PORT_V37_SCHED_OK;
}

uint32_t port_v37_sched_init(struct port_v37_scheduler *s,
    const struct port_v37_sched_ops *ops){
    if(!s||!ops||!ops->address||!ops->resolve)return PORT_V37_SCHED_BADPARAM;
    struct port_v37_scheduler candidate={0};candidate.ops=*ops;
    candidate.current_priority=32u;
    for(unsigned q=0;q<PORT_V37_SCH_QUEUES;q++){
        uint32_t h=ops->address(ops->context,&s->heads[q]);
        if(!h)return PORT_V37_SCHED_BADPARAM;
        candidate.heads[q].forward=h;candidate.heads[q].backward=h;
    }
    candidate.initialized=1u;*s=candidate;
    return PORT_V37_SCHED_OK;
}

uint32_t port_v37_sched_chse(struct port_v37_scheduler *s,uint32_t address,
    uint8_t priority_class){
    if(!s||!s->initialized||priority_class>=PORT_V37_SCH_INCREMENT_CLASSES||
       !address)return PORT_V37_SCHED_BADPARAM;
    uint8_t *pcb=resolve(s,address,PORT_V37_PCB_BYTES);
    if(!pcb)return PORT_V37_SCHED_UNRESOLVED;
    if(get16(pcb+PORT_V37_PCB_SIZE)!=PORT_V37_PCB_BYTES||
       pcb[Q_TYPE]!=PORT_V37_DYN_PCB||
       pcb[PCBPRI]>=PORT_V37_SCH_PRIORITIES||
       pcb[PCBPRIB]>=PORT_V37_SCH_PRIORITIES)
        return PORT_V37_SCHED_CORRUPT;
    const uint16_t old_state=get16(pcb+PCBSTATE);
    const uint8_t old_priority=pcb[PCBPRI];
    if(old_state==PORT_V37_SCH_COM||old_state==PORT_V37_SCH_COMO){
        const unsigned old_queue=old_priority+
            (old_state==PORT_V37_SCH_COMO?32u:0u);
        int found=0;
        uint32_t status=list_contains(s,old_queue,address,&found);
        if(status!=PORT_V37_SCHED_OK||!found)return PORT_V37_SCHED_CORRUPT;
    }
    uint32_t status=list_remove(s,address,pcb);
    if(status!=PORT_V37_SCHED_OK)return status;
    if(old_state==PORT_V37_SCH_COM||old_state==PORT_V37_SCH_COMO){
        const unsigned old_queue=old_priority+
            (old_state==PORT_V37_SCH_COMO?32u:0u);
        if(list_empty(s,old_queue))set_summary(s,old_queue,0);
        if(old_queue<32u&&s->ready_count[old_queue])s->ready_count[old_queue]--;
    }
    int candidate=(int)pcb[PCBPRIB]-(int)priority_increment[priority_class];
    if(candidate>(int)old_priority)candidate=old_priority;
    if(candidate<16)candidate=pcb[PCBPRIB];
    if(candidate<0||candidate>=32)return PORT_V37_SCHED_CORRUPT;
    pcb[PCBPRI]=(uint8_t)candidate;
    const int resident=(get32(pcb+PCBSTS)&PORT_V37_SCH_RESIDENT)!=0;
    const uint16_t new_state=resident?PORT_V37_SCH_COM:PORT_V37_SCH_COMO;
    const unsigned new_queue=(unsigned)candidate+(resident?0u:32u);
    put16(pcb+PCBSTATE,new_state);
    status=list_insert_tail(s,new_queue,address);
    if(status!=PORT_V37_SCHED_OK)return status;
    set_summary(s,new_queue,1);
    if(resident)s->ready_count[candidate]++;
    else if(s->ops.wake_swapper)s->ops.wake_swapper(s->ops.context,address);
    if(resident&&(uint32_t)candidate<=s->current_priority&&
       s->ops.request_reschedule)
        s->ops.request_reschedule(s->ops.context);
    return PORT_V37_SCHED_OK;
}

uint32_t port_v37_sched_select(struct port_v37_scheduler *s,
                               uint32_t *selected_address){
    if(!s||!s->initialized||!selected_address)return PORT_V37_SCHED_BADPARAM;
    *selected_address=0u;
    uint32_t bits=s->summary[0];
    if(!bits)return PORT_V37_SCHED_EMPTY;
    unsigned q=0u;
    while(!(bits&1u)){bits>>=1;++q;}
    const uint32_t head=head_address(s,q);
    const uint32_t address=s->heads[q].forward;
    uint8_t *pcb=resolve(s,address,PORT_V37_PCB_BYTES);
    if(!head||address==head||!pcb)return PORT_V37_SCHED_CORRUPT;
    int found=0;
    if(list_contains(s,q,address,&found)!=PORT_V37_SCHED_OK||!found||
       get16(pcb+PORT_V37_PCB_SIZE)!=PORT_V37_PCB_BYTES||
       pcb[Q_TYPE]!=PORT_V37_DYN_PCB||
       get16(pcb+PCBSTATE)!=PORT_V37_SCH_COM||pcb[PCBPRI]!=q)
        return PORT_V37_SCHED_CORRUPT;
    /* SCH$SCHED executes LDPCTX after selecting. A PCB without its original
     * PHD and native processor context cannot be made current on IA-32. */
    if(!get32(pcb+PORT_V37_PCB_PHD)||!get32(pcb+PORT_V37_PCB_PHYPCB))
        return PORT_V37_SCHED_NO_CONTEXT;
    if(!s->ready_count[q])return PORT_V37_SCHED_CORRUPT;
    uint32_t status=list_remove(s,address,pcb);
    if(status!=PORT_V37_SCHED_OK)return status;
    if(list_empty(s,q))set_summary(s,q,0);
    s->ready_count[q]--;
    put16(pcb+PCBSTATE,PORT_V37_SCH_CUR);
    uint8_t priority=pcb[PCBPRI];
    if(priority!=pcb[PCBPRIB]&&(priority&0x10u)){
        priority=(uint8_t)(priority+1u);
        pcb[PCBPRI]=priority;
    }
    s->current_pcb=address;
    s->current_priority=priority;
    *selected_address=address;
    return PORT_V37_SCHED_OK;
}

uint32_t port_v37_sched_check(const struct port_v37_scheduler *s){
    if(!s||!s->initialized)return PORT_V37_SCHED_BADPARAM;
    uint32_t summary[2]={0u,0u};uint32_t ready[32]={0u};
    for(unsigned q=0;q<PORT_V37_SCH_QUEUES;q++){
        const uint32_t h=head_address(s,q);
        if(!h||!resolve(s,h,sizeof(struct port_v37_sched_head)))
            return PORT_V37_SCHED_UNRESOLVED;
        if(s->heads[q].forward==h){
            if(s->heads[q].backward!=h)return PORT_V37_SCHED_CORRUPT;
            continue;
        }
        summary[q>>5]|=1u<<(q&31u);
        int found;
        uint32_t rc=list_contains(s,q,h,&found);
        if(rc!=PORT_V37_SCHED_OK||found)return PORT_V37_SCHED_CORRUPT;
        uint32_t cursor=s->heads[q].forward;unsigned count=0u;
        while(cursor!=h){
            uint8_t *p=resolve(s,cursor,PORT_V37_PCB_BYTES);
            if(!p||get16(p+PCBSTATE)!=(q<32u?PORT_V37_SCH_COM:PORT_V37_SCH_COMO)||
               p[PCBPRI]!=(q&31u))return PORT_V37_SCHED_CORRUPT;
            if(q<32u)ready[q]++;
            if(++count>8192u)return PORT_V37_SCHED_CORRUPT;
            cursor=get32(p+SQFL);
        }
    }
    if(summary[0]!=s->summary[0]||summary[1]!=s->summary[1])
        return PORT_V37_SCHED_CORRUPT;
    for(unsigned i=0;i<32u;i++)if(ready[i]!=s->ready_count[i])
        return PORT_V37_SCHED_CORRUPT;
    return PORT_V37_SCHED_OK;
}
