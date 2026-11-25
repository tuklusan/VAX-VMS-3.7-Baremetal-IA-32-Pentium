#include "vms_v37_sched.h"
#include "vms_v37_dyn_tags.h"
#include "vms_v37_ia32_context.h"
#include "vms_v37_phd.h"

_Static_assert(sizeof(struct port_v37_wait_queue)==12u,
               "SYSDEFQZ.MDL WQH source layout");

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
static int source_wait_state(uint16_t state){
    return state>=PORT_V37_SCH_COLPG&&state<=PORT_V37_SCH_FPG;
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

static uint32_t wait_queue_check(const struct port_v37_scheduler *s,
    uint32_t address,uint8_t **queue_out){
    if(address&3u)return PORT_V37_SCHED_BADPARAM;
    uint8_t *queue=resolve(s,address,sizeof(struct port_v37_wait_queue));
    if(!queue)return PORT_V37_SCHED_UNRESOLVED;
    const uint32_t first=get32(queue),last=get32(queue+4u);
    const uint16_t expected=get16(queue+8u),state=get16(queue+10u);
    if(!source_wait_state(state))return PORT_V37_SCHED_CORRUPT;
    uint32_t cursor=first,previous=address,seen=0u;
    while(cursor!=address){
        if(seen>=expected||seen>=8192u)return PORT_V37_SCHED_CORRUPT;
        uint8_t *pcb=resolve(s,cursor,PORT_V37_PCB_BYTES);
        if(!pcb||get16(pcb+PORT_V37_PCB_SIZE)!=PORT_V37_PCB_BYTES||
           pcb[Q_TYPE]!=PORT_V37_DYN_PCB||get32(pcb+SQBL)!=previous||
           get16(pcb+PCBSTATE)!=state)return PORT_V37_SCHED_CORRUPT;
        previous=cursor;cursor=get32(pcb+SQFL);seen++;
    }
    if(seen!=expected||previous!=last||get32(queue+4u)!=previous||
       (expected==0u&&(first!=address||last!=address)))
        return PORT_V37_SCHED_CORRUPT;
    if(queue_out)*queue_out=queue;
    return PORT_V37_SCHED_OK;
}

static uint32_t wait_queue_insert_tail(const struct port_v37_scheduler *s,
    uint32_t address,uint8_t *queue,uint32_t pcb_address,uint8_t *pcb){
    const uint32_t tail=get32(queue+4u);
    uint8_t *tail_record=resolve(s,tail,sizeof(struct port_v37_sched_head));
    if(!tail_record)return PORT_V37_SCHED_UNRESOLVED;
    put32(pcb+SQFL,address);put32(pcb+SQBL,tail);
    put32(tail_record+SQFL,pcb_address);put32(queue+4u,pcb_address);
    if(get32(queue)==address)put32(queue,pcb_address);
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

uint32_t port_v37_sched_set_priority(struct port_v37_scheduler *s,
    uint32_t address,uint8_t priority){
    if(!s||!s->initialized||!address||priority>=PORT_V37_SCH_PRIORITIES)
        return PORT_V37_SCHED_BADPARAM;
    uint8_t *pcb=resolve(s,address,PORT_V37_PCB_BYTES);
    if(!pcb)return PORT_V37_SCHED_UNRESOLVED;
    const uint16_t old_state=get16(pcb+PCBSTATE);
    const uint8_t old_priority=pcb[PCBPRI];
    const int resident=(get32(pcb+PCBSTS)&PORT_V37_SCH_RESIDENT)!=0;
    if(get16(pcb+PORT_V37_PCB_SIZE)!=PORT_V37_PCB_BYTES||
       pcb[Q_TYPE]!=PORT_V37_DYN_PCB||old_priority>=32u||
       (resident?old_state!=PORT_V37_SCH_COM:
                 old_state!=PORT_V37_SCH_COMO))
        return PORT_V37_SCHED_CORRUPT;
    const unsigned old_queue=(unsigned)old_priority+(resident?0u:32u);
    const unsigned new_queue=(unsigned)priority+(resident?0u:32u);
    int found=0;
    uint32_t status=list_contains(s,old_queue,address,&found);
    if(status!=PORT_V37_SCHED_OK||!found)return PORT_V37_SCHED_CORRUPT;
    status=list_contains(s,new_queue,UINT32_MAX,&found);
    if(status!=PORT_V37_SCHED_OK)return status;
    if(!resolve(s,s->heads[old_queue].backward,
                sizeof(struct port_v37_sched_head))||
       !resolve(s,s->heads[new_queue].backward,
                sizeof(struct port_v37_sched_head)))
        return PORT_V37_SCHED_UNRESOLVED;
    if(resident&&!s->ready_count[old_priority])
        return PORT_V37_SCHED_CORRUPT;
    status=list_remove(s,address,pcb);
    if(status!=PORT_V37_SCHED_OK)return status;
    if(list_empty(s,old_queue))set_summary(s,old_queue,0);
    if(resident)s->ready_count[old_priority]--;
    pcb[PCBPRI]=priority;
    status=list_insert_tail(s,new_queue,address);
    if(status!=PORT_V37_SCHED_OK)return PORT_V37_SCHED_CORRUPT;
    set_summary(s,new_queue,1);
    if(resident){
        s->ready_count[priority]++;
        if((uint32_t)priority<=s->current_priority&&
           s->ops.request_reschedule)
            s->ops.request_reschedule(s->ops.context);
    }else if(s->ops.wake_swapper)
        s->ops.wake_swapper(s->ops.context,address);
    return PORT_V37_SCHED_OK;
}

uint32_t port_v37_sched_select(struct port_v37_scheduler *s,
                               uint32_t *selected_address,
                               uint32_t *restore_frame){
    if(!s||!s->initialized||!selected_address||!restore_frame)
        return PORT_V37_SCHED_BADPARAM;
    *selected_address=0u;*restore_frame=0u;
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
    const uint32_t phy_pcb=get32(pcb+PORT_V37_PCB_PHYPCB);
    struct port_v37_ia32_restore_frame *const frame=
        (struct port_v37_ia32_restore_frame *)resolve(s,phy_pcb,
            sizeof(struct port_v37_ia32_restore_frame));
    if(!get32(pcb+PORT_V37_PCB_PHD)||!frame)
        return PORT_V37_SCHED_NO_CONTEXT;
    if(port_v37_ia32_context_validate(frame)!=PORT_V37_IA32_CONTEXT_OK)
        return PORT_V37_SCHED_CORRUPT;
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
    *restore_frame=phy_pcb;
    return PORT_V37_SCHED_OK;
}

uint32_t port_v37_sched_resched_current(struct port_v37_scheduler *s,
                                        uint32_t address){
    if(!s||!s->initialized||!address||s->current_pcb!=address)
        return PORT_V37_SCHED_BADPARAM;
    uint8_t *pcb=resolve(s,address,PORT_V37_PCB_BYTES);
    if(!pcb)return PORT_V37_SCHED_UNRESOLVED;
    if(get16(pcb+PORT_V37_PCB_SIZE)!=PORT_V37_PCB_BYTES||
       pcb[Q_TYPE]!=PORT_V37_DYN_PCB||
       get16(pcb+PCBSTATE)!=PORT_V37_SCH_CUR||pcb[PCBPRI]>=32u||
       !(get32(pcb+PCBSTS)&PORT_V37_SCH_RESIDENT)||
       get32(pcb+SQFL)!=address||get32(pcb+SQBL)!=address)
        return PORT_V37_SCHED_CORRUPT;
    const unsigned q=pcb[PCBPRI];
    uint32_t status=list_insert_tail(s,q,address);
    if(status!=PORT_V37_SCHED_OK)return status;
    put16(pcb+PCBSTATE,PORT_V37_SCH_COM);
    set_summary(s,q,1);
    s->ready_count[q]++;
    s->current_pcb=0u;
    s->current_priority=PORT_V37_SCH_PRIORITIES;
    return PORT_V37_SCHED_OK;
}

uint32_t port_v37_sched_save_current(struct port_v37_scheduler *s,
                                      uint32_t frame_address){
    if(!s||!s->initialized||!s->current_pcb||!frame_address)
        return PORT_V37_SCHED_BADPARAM;
    uint8_t *const pcb=resolve(s,s->current_pcb,PORT_V37_PCB_BYTES);
    struct port_v37_ia32_restore_frame *const frame=
        (struct port_v37_ia32_restore_frame *)resolve(s,frame_address,
            sizeof(struct port_v37_ia32_restore_frame));
    if(!pcb||!frame||get16(pcb+PORT_V37_PCB_STATE)!=PORT_V37_SCH_CUR||
       port_v37_ia32_context_validate(frame)!=PORT_V37_IA32_CONTEXT_OK)
        return PORT_V37_SCHED_CORRUPT;
    const uint32_t phd_address=get32(pcb+PORT_V37_PCB_PHD);
    uint8_t *const phd=resolve(s,phd_address,PORT_V37_PHD_BYTES);
    if(!phd)return PORT_V37_SCHED_NO_CONTEXT;
    const uint32_t old_frame=get32(pcb+PORT_V37_PCB_PHYPCB);
    const uint32_t old_ksp=get32(phd+PORT_V37_PHD_KSP);
    const uint32_t old_pc=get32(phd+PORT_V37_PHD_PC);
    put32(pcb+PORT_V37_PCB_PHYPCB,frame_address);
    put32(phd+PORT_V37_PHD_KSP,frame_address);
    put32(phd+PORT_V37_PHD_PC,frame->registers.eip);
    const uint32_t status=port_v37_sched_resched_current(s,s->current_pcb);
    if(status!=PORT_V37_SCHED_OK){
        put32(pcb+PORT_V37_PCB_PHYPCB,old_frame);
        put32(phd+PORT_V37_PHD_KSP,old_ksp);
        put32(phd+PORT_V37_PHD_PC,old_pc);
    }
    return status;
}

uint32_t port_v37_sched_interrupt(struct port_v37_scheduler *s,
    uint32_t interrupted_frame,uint32_t *selected_pcb,
    uint32_t *selected_frame){
    if(!s||!selected_pcb||!selected_frame)
        return PORT_V37_SCHED_BADPARAM;
    if(s->native_interrupt_count!=UINT32_MAX)s->native_interrupt_count++;
    *selected_pcb=0u;*selected_frame=0u;
    if(s->current_pcb){
        const uint32_t saved=port_v37_sched_save_current(s,interrupted_frame);
        if(saved!=PORT_V37_SCHED_OK){
            if(saved==PORT_V37_SCHED_NO_CONTEXT&&
               s->native_no_context_count!=UINT32_MAX)
                s->native_no_context_count++;
            return saved;
        }
    }
    const uint32_t selected=port_v37_sched_select(s,selected_pcb,
                                                  selected_frame);
    if(selected==PORT_V37_SCHED_NO_CONTEXT&&
       s->native_no_context_count!=UINT32_MAX)
        s->native_no_context_count++;
    return selected;
}

uint32_t port_v37_sched_wait_current(struct port_v37_scheduler *s,
    uint32_t wait_address,uint32_t interrupted_frame,
    uint32_t *selected_pcb,uint32_t *selected_frame){
    if(!s||!s->initialized||!wait_address||!interrupted_frame||
       !selected_pcb||!selected_frame||!s->current_pcb)
        return PORT_V37_SCHED_BADPARAM;
    *selected_pcb=0u;*selected_frame=0u;
    uint8_t *queue=0;
    uint32_t status=wait_queue_check(s,wait_address,&queue);
    if(status!=PORT_V37_SCHED_OK)return status;
    if(get16(queue+8u)==UINT16_MAX)return PORT_V37_SCHED_CORRUPT;
    uint8_t *pcb=resolve(s,s->current_pcb,PORT_V37_PCB_BYTES);
    const uint32_t pcb_address=s->current_pcb;
    if(!pcb||get16(pcb+PCBSTATE)!=PORT_V37_SCH_CUR||
       get32(pcb+SQFL)!=pcb_address||get32(pcb+SQBL)!=pcb_address)
        return PORT_V37_SCHED_CORRUPT;
    struct port_v37_ia32_restore_frame *frame=
        (struct port_v37_ia32_restore_frame *)resolve(s,interrupted_frame,
            sizeof(struct port_v37_ia32_restore_frame));
    const uint32_t phd_address=get32(pcb+PORT_V37_PCB_PHD);
    uint8_t *phd=resolve(s,phd_address,PORT_V37_PHD_BYTES);
    if(!frame||!phd)return PORT_V37_SCHED_NO_CONTEXT;
    if(port_v37_ia32_context_validate(frame)!=PORT_V37_IA32_CONTEXT_OK)
        return PORT_V37_SCHED_CORRUPT;

    /* Preflight the wait-queue tail before changing the running process. */
    if(!resolve(s,get32(queue+4u),sizeof(struct port_v37_sched_head)))
        return PORT_V37_SCHED_UNRESOLVED;
    const uint32_t old_phy=get32(pcb+PORT_V37_PCB_PHYPCB);
    const uint32_t old_ksp=get32(phd+PORT_V37_PHD_KSP);
    const uint32_t old_pc=get32(phd+PORT_V37_PHD_PC);
    put32(pcb+PORT_V37_PCB_PHYPCB,interrupted_frame);
    put32(phd+PORT_V37_PHD_KSP,interrupted_frame);
    put32(phd+PORT_V37_PHD_PC,frame->registers.eip);
    status=wait_queue_insert_tail(s,wait_address,queue,pcb_address,pcb);
    if(status!=PORT_V37_SCHED_OK){
        put32(pcb+PORT_V37_PCB_PHYPCB,old_phy);
        put32(phd+PORT_V37_PHD_KSP,old_ksp);
        put32(phd+PORT_V37_PHD_PC,old_pc);
        return status;
    }
    put16(queue+8u,(uint16_t)(get16(queue+8u)+1u));
    put16(pcb+PCBSTATE,get16(queue+10u));
    s->current_pcb=0u;s->current_priority=32u;
    status=port_v37_sched_select(s,selected_pcb,selected_frame);
    /* SCH$WAITK cannot return to the process it just blocked. Surface a
     * committed blocked state when no native runnable context is available. */
    return status==PORT_V37_SCHED_EMPTY||
           status==PORT_V37_SCHED_NO_CONTEXT?
        PORT_V37_SCHED_BLOCKED:status;
}

uint32_t port_v37_sched_wake_waiter(struct port_v37_scheduler *s,
    uint32_t wait_address,uint32_t pcb_address,uint8_t priority_class){
    if(!s||!s->initialized||!wait_address||!pcb_address||
       priority_class>=PORT_V37_SCH_INCREMENT_CLASSES)
        return PORT_V37_SCHED_BADPARAM;
    uint8_t *queue=0;
    uint32_t status=wait_queue_check(s,wait_address,&queue);
    if(status!=PORT_V37_SCHED_OK)return status;
    uint8_t *pcb=resolve(s,pcb_address,PORT_V37_PCB_BYTES);
    if(!pcb)return PORT_V37_SCHED_UNRESOLVED;
    if(!get16(queue+8u)||get16(pcb+PCBSTATE)!=get16(queue+10u))
        return PORT_V37_SCHED_CORRUPT;
    uint32_t cursor=get32(queue),previous=wait_address;
    int found=0;
    for(unsigned n=0;n<get16(queue+8u);n++){
        uint8_t *item=resolve(s,cursor,PORT_V37_PCB_BYTES);
        if(!item||get32(item+SQBL)!=previous)
            return PORT_V37_SCHED_CORRUPT;
        if(cursor==pcb_address){found=1;break;}
        previous=cursor;cursor=get32(item+SQFL);
    }
    if(!found)return PORT_V37_SCHED_CORRUPT;

    /* CHSE is guaranteed to have a valid target tail before unlinking WQH. */
    const uint8_t old_priority=pcb[PCBPRI],base_priority=pcb[PCBPRIB];
    int candidate=(int)base_priority-(int)priority_increment[priority_class];
    if(candidate>(int)old_priority)candidate=old_priority;
    if(candidate<16)candidate=base_priority;
    if(candidate<0||candidate>=32)return PORT_V37_SCHED_CORRUPT;
    const unsigned new_queue=(unsigned)candidate+
        ((get32(pcb+PCBSTS)&PORT_V37_SCH_RESIDENT)?0u:32u);
    int ignored=0;
    status=list_contains(s,new_queue,UINT32_MAX,&ignored);
    if(status!=PORT_V37_SCHED_OK)return status;
    if(!resolve(s,s->heads[new_queue].backward,
                sizeof(struct port_v37_sched_head)))
        return PORT_V37_SCHED_UNRESOLVED;

    const uint32_t old_next=get32(pcb+SQFL),old_previous=get32(pcb+SQBL);
    const uint16_t old_state=get16(pcb+PCBSTATE);
    status=list_remove(s,pcb_address,pcb);
    if(status!=PORT_V37_SCHED_OK)return status;
    put16(queue+8u,(uint16_t)(get16(queue+8u)-1u));
    put16(pcb+PCBSTATE,0u);
    status=port_v37_sched_chse(s,pcb_address,priority_class);
    if(status==PORT_V37_SCHED_OK)return status;

    /* A late CHSE failure restores the exact wait-list position and state. */
    uint8_t *next=resolve(s,old_next,sizeof(struct port_v37_sched_head));
    uint8_t *prev=resolve(s,old_previous,sizeof(struct port_v37_sched_head));
    if(next&&prev){
        put32(pcb+SQFL,old_next);put32(pcb+SQBL,old_previous);
        put32(prev+SQFL,pcb_address);put32(next+SQBL,pcb_address);
        put16(queue+8u,(uint16_t)(get16(queue+8u)+1u));
        put16(pcb+PCBSTATE,old_state);
        pcb[PCBPRI]=old_priority;
    }
    return status;
}

uint32_t port_v37_sched_rse_event(struct port_v37_scheduler *s,
    uint8_t event,uint32_t wait_address,uint32_t pcb_address,
    uint8_t priority_class){
    if(!s||!s->initialized||!pcb_address)
        return PORT_V37_SCHED_BADPARAM;
    if(event==PORT_V37_RSE_SETPRI){
        if(priority_class>=PORT_V37_SCH_PRIORITIES)
            return PORT_V37_SCHED_BADPARAM;
        const uint8_t *pcb=resolve(s,pcb_address,PORT_V37_PCB_BYTES);
        if(!pcb)return PORT_V37_SCHED_UNRESOLVED;
        const uint16_t state=get16(pcb+PCBSTATE);
        if(state!=PORT_V37_SCH_COM&&state!=PORT_V37_SCH_COMO)
            return PORT_V37_SCHED_IGNORED;
        return port_v37_sched_set_priority(s,pcb_address,priority_class);
    }
    if(!wait_address||priority_class>=PORT_V37_SCH_INCREMENT_CLASSES)
        return PORT_V37_SCHED_BADPARAM;
    uint16_t states=0u;
    uint8_t complete_wait=0u;
    switch(event){
    case PORT_V37_RSE_AST: states=(uint16_t)((1u<<PORT_V37_SCH_CEF)|
        (1u<<PORT_V37_SCH_COLPG)|(1u<<PORT_V37_SCH_FPG)|
        (1u<<PORT_V37_SCH_HIB)|(1u<<PORT_V37_SCH_HIBO)|
        (1u<<PORT_V37_SCH_LEF)|(1u<<PORT_V37_SCH_LEFO)|
        (1u<<PORT_V37_SCH_MWAIT)|(1u<<PORT_V37_SCH_PFW));break;
    case PORT_V37_RSE_EVENT_LEF: states=(uint16_t)(1u<<5);complete_wait=1u;break;
    case PORT_V37_RSE_EVENT_CEF: states=(uint16_t)(1u<<3);complete_wait=1u;break;
    case PORT_V37_RSE_EVENT_LEFO: states=(uint16_t)(1u<<6);break;
    case PORT_V37_RSE_FPGA: states=(uint16_t)(1u<<11);break; /* FPG */
    case PORT_V37_RSE_WAKE: states=(uint16_t)((1u<<7)|(1u<<8));break;
    case PORT_V37_RSE_RESUME: states=(uint16_t)((1u<<9)|(1u<<10));break;
    case PORT_V37_RSE_PFCOM: states=(uint16_t)(1u<<4);break; /* PFW */
    default:return PORT_V37_SCHED_UNRESOLVED;
    }
    const uint8_t *pcb=resolve(s,pcb_address,PORT_V37_PCB_BYTES);
    if(!pcb)return PORT_V37_SCHED_UNRESOLVED;
    const uint16_t state=get16(pcb+PCBSTATE);
    if(state>=16u||!(states&(uint16_t)(1u<<state)))
        return PORT_V37_SCHED_IGNORED;
    uint8_t *phd=0;uint32_t old_pc=0u,old_r0=0u,old_eax=0u;
    struct port_v37_ia32_restore_frame *frame=0;
    if(complete_wait){
        const uint32_t phd_address=get32(pcb+PORT_V37_PCB_PHD);
        const uint32_t frame_address=get32(pcb+PORT_V37_PCB_PHYPCB);
        phd=resolve(s,phd_address,PORT_V37_PHD_BYTES);
        frame=(struct port_v37_ia32_restore_frame *)resolve(s,frame_address,
            sizeof(struct port_v37_ia32_restore_frame));
        if(!phd||!frame)return PORT_V37_SCHED_NO_CONTEXT;
        if(port_v37_ia32_context_validate(frame)!=PORT_V37_IA32_CONTEXT_OK)
            return PORT_V37_SCHED_CORRUPT;
        old_pc=get32(phd+PORT_V37_PHD_PC);
        old_r0=get32(phd+PORT_V37_PHD_R0);
        old_eax=frame->registers.eax;
        /* The IA-32 interrupt frame already contains the address after INT.
         * PHD PC mirrors that native return address; R0 maps to saved EAX. */
        put32(phd+PORT_V37_PHD_PC,frame->registers.eip);
        put32(phd+PORT_V37_PHD_R0,1u); /* SS$_NORMAL */
        frame->registers.eax=1u;
    }
    const uint32_t status=port_v37_sched_wake_waiter(s,wait_address,
        pcb_address,priority_class);
    if(status!=PORT_V37_SCHED_OK&&phd){
        put32(phd+PORT_V37_PHD_PC,old_pc);
        put32(phd+PORT_V37_PHD_R0,old_r0);
        frame->registers.eax=old_eax;
    }
    return status;
}

static uint32_t event_records(struct port_v37_scheduler *s,uint8_t *pcb,
    uint32_t pcb_address,uint32_t local_wqh,unsigned cluster,
    uint32_t *flags_address,uint32_t *wqh_address,uint16_t *wait_state,
    uint8_t **ceb_out){
    if(cluster<2u){
        *flags_address=pcb_address+PORT_V37_PCB_EFCS+cluster*4u;
        *wqh_address=local_wqh;*wait_state=PORT_V37_SCH_LEF;
        if(ceb_out)*ceb_out=0;
        return local_wqh?PORT_V37_SCHED_OK:PORT_V37_SCHED_BADPARAM;
    }
    const uint32_t ceb_address=get32(pcb+PORT_V37_PCB_EFC2P+
                                     (cluster-2u)*4u);
    uint8_t *ceb=resolve(s,ceb_address,PORT_V37_CEB_BYTES);
    if(!ceb)return PORT_V37_SCHED_UNRESOLVED;
    const uint8_t type=ceb[PORT_V37_CEB_TYPE];
    if(type!=PORT_V37_CEB_NORMAL&&type!=PORT_V37_CEB_SLAVE&&
       type!=PORT_V37_CEB_SHARED)return PORT_V37_SCHED_CORRUPT;
    if(ceb_out)*ceb_out=ceb;
    *flags_address=ceb_address+PORT_V37_CEB_EFC;
    *wqh_address=ceb_address+PORT_V37_CEB_WQH;
    *wait_state=PORT_V37_SCH_CEF;
    return PORT_V37_SCHED_OK;
}

static int event_condition(const uint8_t *pcb,uint32_t flags,int wait_all){
    const uint32_t selected=~get32(pcb+PORT_V37_PCB_EFWM);
    const uint32_t present=flags&selected;
    return wait_all?present==selected:present!=0u;
}

uint32_t port_v37_sched_wait_event(struct port_v37_scheduler *s,
    uint32_t local_lef_wqh,uint32_t interrupted_frame,uint32_t efn,
    uint32_t mask,int wait_all,uint32_t *selected_pcb,
    uint32_t *selected_frame,uint32_t *service_status){
    if(!s||!s->initialized||!s->current_pcb||!interrupted_frame||
       !selected_pcb||!selected_frame||!service_status)
        return PORT_V37_SCHED_BADPARAM;
    *selected_pcb=0u;*selected_frame=0u;
    if(efn>127u){*service_status=PORT_V37_SS_ILLEFC;
        return PORT_V37_SCHED_OK;}
    const unsigned cluster=efn>>5u;
    const uint32_t pcb_address=s->current_pcb;
    uint8_t *pcb=resolve(s,pcb_address,PORT_V37_PCB_BYTES);
    if(!pcb)return PORT_V37_SCHED_UNRESOLVED;
    uint32_t flags_address=0u,wqh_address=0u;uint16_t state=0u;
    uint8_t *ceb=0;
    uint32_t status=event_records(s,pcb,pcb_address,local_lef_wqh,
        cluster,&flags_address,&wqh_address,&state,&ceb);
    if(status!=PORT_V37_SCHED_OK){
        *service_status=status==PORT_V37_SCHED_UNRESOLVED?
            PORT_V37_SS_UNASEFC:PORT_V37_SS_ILLEFC;
        return status==PORT_V37_SCHED_UNRESOLVED?PORT_V37_SCHED_OK:status;
    }
    /* Slave CEB replication requires the original interprocessor request
     * and lock path; reject it until that path is present. */
    if(ceb&&ceb[PORT_V37_CEB_TYPE]==PORT_V37_CEB_SLAVE){
        *service_status=PORT_V37_SS_UNASEFC;return PORT_V37_SCHED_OK;
    }
    uint8_t *flags=resolve(s,flags_address,sizeof(uint32_t)),*queue=0;
    status=wait_queue_check(s,wqh_address,&queue);
    if(!flags||status!=PORT_V37_SCHED_OK)
        return status==PORT_V37_SCHED_OK?PORT_V37_SCHED_UNRESOLVED:status;
    if(get16(queue+10u)!=state)return PORT_V37_SCHED_CORRUPT;
    const uint32_t current=__atomic_load_n((uint32_t *)(void *)flags,
                                           __ATOMIC_SEQ_CST);
    const int satisfied=wait_all?((current&mask)==mask):((current&mask)!=0u);
    *service_status=1u;
    if(satisfied)return PORT_V37_SCHED_OK;
    const uint32_t old_mask=get32(pcb+PORT_V37_PCB_EFWM);
    const uint8_t old_cluster=pcb[PORT_V37_PCB_WEFC];
    const uint32_t old_sts=get32(pcb+PORT_V37_PCB_STS);
    const uint32_t wait_mask=wait_all?(mask&~current):mask;
    put32(pcb+PORT_V37_PCB_EFWM,~wait_mask);
    pcb[PORT_V37_PCB_WEFC]=(uint8_t)cluster;
    put32(pcb+PORT_V37_PCB_STS,wait_all?
        old_sts|PORT_V37_PCB_STS_WALL:old_sts&~PORT_V37_PCB_STS_WALL);
    const uint16_t old_count=get16(queue+8u);
    status=port_v37_sched_wait_current(s,wqh_address,interrupted_frame,
        selected_pcb,selected_frame);
    if(get16(queue+8u)==old_count&&
       get16(pcb+PORT_V37_PCB_STATE)==PORT_V37_SCH_CUR){
        put32(pcb+PORT_V37_PCB_EFWM,old_mask);
        pcb[PORT_V37_PCB_WEFC]=old_cluster;
        put32(pcb+PORT_V37_PCB_STS,old_sts);
    }
    return status;
}

uint32_t port_v37_sched_post_event(struct port_v37_scheduler *s,
    uint32_t target_pcb,uint32_t local_lef_wqh,uint32_t efn,
    uint8_t priority_class,uint32_t *service_status){
    if(!s||!s->initialized||!target_pcb||!service_status||
       priority_class>=PORT_V37_SCH_INCREMENT_CLASSES)
        return PORT_V37_SCHED_BADPARAM;
    if(efn>127u){*service_status=PORT_V37_SS_ILLEFC;
        return PORT_V37_SCHED_OK;}
    uint8_t *target=resolve(s,target_pcb,PORT_V37_PCB_BYTES);
    if(!target)return PORT_V37_SCHED_UNRESOLVED;
    const unsigned cluster=efn>>5u;
    uint32_t flags_address=0u,wqh_address=0u;uint16_t wait_state=0u;
    uint8_t *ceb=0;
    uint32_t status=event_records(s,target,target_pcb,local_lef_wqh,
        cluster,&flags_address,&wqh_address,&wait_state,&ceb);
    if(status!=PORT_V37_SCHED_OK){
        *service_status=status==PORT_V37_SCHED_UNRESOLVED?
            PORT_V37_SS_UNASEFC:PORT_V37_SS_ILLEFC;
        return status==PORT_V37_SCHED_UNRESOLVED?PORT_V37_SCHED_OK:status;
    }
    if(ceb&&ceb[PORT_V37_CEB_TYPE]==PORT_V37_CEB_SLAVE){
        *service_status=PORT_V37_SS_UNASEFC;return PORT_V37_SCHED_OK;
    }
    uint8_t *flags=resolve(s,flags_address,sizeof(uint32_t)),*queue=0;
    status=wait_queue_check(s,wqh_address,&queue);
    if(!flags||status!=PORT_V37_SCHED_OK)
        return status==PORT_V37_SCHED_OK?PORT_V37_SCHED_UNRESOLVED:status;
    if(get16(queue+10u)!=wait_state)return PORT_V37_SCHED_CORRUPT;
    const uint32_t bit=UINT32_C(1)<<(efn&31u);
    const uint32_t previous_flags=__atomic_fetch_or(
        (uint32_t *)(void *)flags,bit,__ATOMIC_SEQ_CST);
    *service_status=(previous_flags&bit)?PORT_V37_SS_WASSET:1u;
    if(cluster<2u){
        if(target[PORT_V37_PCB_WEFC]==cluster&&
           get16(target+PORT_V37_PCB_STATE)==PORT_V37_SCH_LEF&&
           event_condition(target,previous_flags|bit,
             (get32(target+PORT_V37_PCB_STS)&PORT_V37_PCB_STS_WALL)!=0u))
            return port_v37_sched_rse_event(s,PORT_V37_RSE_EVENT_LEF,
                wqh_address,target_pcb,priority_class);
        return PORT_V37_SCHED_OK;
    }
    uint32_t cursor=get32(queue);
    const unsigned count=get16(queue+8u);
    for(unsigned n=0;n<count;n++){
        uint8_t *pcb=resolve(s,cursor,PORT_V37_PCB_BYTES);
        if(!pcb)return PORT_V37_SCHED_CORRUPT;
        const uint32_t next=get32(pcb+PORT_V37_PCB_SQFL);
        if((get32(pcb+PORT_V37_PCB_STS)&PORT_V37_PCB_STS_RESIDENT)&&
           pcb[PORT_V37_PCB_WEFC]==cluster&&
           get16(pcb+PORT_V37_PCB_STATE)==PORT_V37_SCH_CEF&&
           event_condition(pcb,previous_flags|bit,
             (get32(pcb+PORT_V37_PCB_STS)&PORT_V37_PCB_STS_WALL)!=0u)){
            status=port_v37_sched_rse_event(s,PORT_V37_RSE_EVENT_CEF,
                wqh_address,cursor,priority_class);
            if(status!=PORT_V37_SCHED_OK)return status;
        }
        cursor=next;
    }
    return wait_queue_check(s,wqh_address,&queue);
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
