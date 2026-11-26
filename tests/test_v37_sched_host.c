#include "vms_v37_sched.h"
#include "vms_v37_ia32_context.h"
#include "vms_v37_phd.h"
#include <stdio.h>
#include <string.h>

struct fixture {
    struct port_v37_scheduler *scheduler;
    uint8_t pcbs[4][PORT_V37_PCB_BYTES];
    struct port_v37_ia32_restore_frame frames[4];
    uint8_t phds[4][PORT_V37_PHD_BYTES];
    struct port_v37_wait_queue wait_queue;
    uint32_t wait_queue_va;
    uint32_t pcb_va[4];
    uint32_t frame_va[4];
    uint32_t phd_va[4];
    uint32_t cr3_by_pix[16],live_cr3;
    unsigned reschedules,swapper_wakes;
};
static uint32_t addr(void *context,const void *p){
    struct fixture *f=context;
    uintptr_t begin=(uintptr_t)f->scheduler;
    uintptr_t end=begin+sizeof *f->scheduler;
    uintptr_t value=(uintptr_t)p;
    if(value>=begin&&value<end)return 0x00100000u+(uint32_t)(value-begin);
    for(unsigned i=0;i<4u;i++)if(p==f->pcbs[i])return f->pcb_va[i];
    if(p==&f->wait_queue)return f->wait_queue_va;
    return 0u;
}
static uint8_t *resolve(void *context,uint32_t va,size_t bytes){
    struct fixture *f=context;
    const uint32_t hb=0x00100000u;
    const uint32_t he=hb+sizeof *f->scheduler;
    if(va>=hb&&va<=he&&bytes<=he-va)
        return (uint8_t *)f->scheduler+(va-hb);
    for(unsigned i=0;i<4u;i++)
        if(va==f->pcb_va[i]&&bytes<=PORT_V37_PCB_BYTES)return f->pcbs[i];
    for(unsigned i=0;i<4u;i++)
        if(va==f->frame_va[i]&&
           bytes<=sizeof(struct port_v37_ia32_restore_frame))
            return (uint8_t *)&f->frames[i];
    for(unsigned i=0;i<4u;i++)
        if(va==f->phd_va[i]&&bytes<=sizeof f->phds[i])
            return f->phds[i];
    if(va==f->wait_queue_va&&bytes<=sizeof f->wait_queue)
        return (uint8_t *)&f->wait_queue;
    return 0;
}
static void wake(void *context,uint32_t pcb){
    struct fixture *f=context;(void)pcb;f->swapper_wakes++;
}
static void reschedule(void *context){
    struct fixture *f=context;f->reschedules++;
}
static uint32_t current_cr3(void *context){
    return ((struct fixture *)context)->live_cr3;
}
static int fail(const char *what){fprintf(stderr,"VMS SCHED FAIL: %s\n",what);return 1;}
static unsigned count(struct fixture *f,unsigned q){
    const uint32_t head=0x00100000u+q*8u;
    uint32_t cursor=f->scheduler->heads[q].forward;unsigned n=0;
    while(cursor!=head&&n<5u){
        n++;uint8_t *p=resolve(f,cursor,PORT_V37_PCB_BYTES);
        if(!p)return 99u;
        cursor=(uint32_t)p[0]|((uint32_t)p[1]<<8)|
            ((uint32_t)p[2]<<16)|((uint32_t)p[3]<<24);
    }
    return cursor==head?n:99u;
}
static uint32_t init_pcb(struct fixture *f,unsigned slot,uint32_t pid,
                         uint8_t priority,int resident){
    const struct port_v37_pcb_seed seed={.address=f->pcb_va[slot],
        .pid=pid,.phd_address=f->phd_va[slot],.priority=priority,
        .name="PROC",.name_bytes=4u};
    if(port_v37_pcb_init_genpcb(f->pcbs[slot],sizeof f->pcbs[slot],&seed)!=
       PORT_V37_PCB_OK)return 0u;
    if(!resident)memset(f->pcbs[slot]+PORT_V37_PCB_STS,0,4u);
    return PORT_V37_PCB_OK;
}
static uint16_t get16(const uint8_t *p){return (uint16_t)p[0]|((uint16_t)p[1]<<8);}
static void put16(uint8_t *p,uint16_t v){p[0]=(uint8_t)v;p[1]=(uint8_t)(v>>8);}
static uint32_t get32(const uint8_t *p){
    return (uint32_t)p[0]|((uint32_t)p[1]<<8)|
       ((uint32_t)p[2]<<16)|((uint32_t)p[3]<<24);
}
static uint64_t get64(const uint8_t *p){
    return (uint64_t)get32(p)|(uint64_t)get32(p+4u)<<32;
}
static void put32(uint8_t *p,uint32_t v){
    p[0]=(uint8_t)v;p[1]=(uint8_t)(v>>8);
    p[2]=(uint8_t)(v>>16);p[3]=(uint8_t)(v>>24);
}

int main(void){
    struct port_v37_scheduler scheduler;struct fixture f={
        .scheduler=&scheduler,.pcb_va={0x00200000u,0x00201000u,
                                       0x00202000u,0x00203000u},
        .frame_va={0x00400000u,0x00401000u,0x00402000u,0x00403000u},
        .phd_va={0x00500000u,0x00501000u,0x00502000u,0x00503000u},
        .wait_queue_va=0x00600000u};
    f.wait_queue.forward=f.wait_queue.backward=f.wait_queue_va;
    f.wait_queue.state=7u;
    const struct port_v37_sched_ops ops={.context=&f,.address=addr,
        .resolve=resolve,.wake_swapper=wake,.request_reschedule=reschedule};
    if(port_v37_sched_init(&scheduler,&ops)!=PORT_V37_SCHED_OK||
       port_v37_sched_check(&scheduler)!=PORT_V37_SCHED_OK)
        return fail("empty 64-queue initialization");
    if(!init_pcb(&f,0u,0x00010000u,0u,1)||
       !init_pcb(&f,1u,0x00010001u,16u,1)||
       !init_pcb(&f,2u,0x00010002u,16u,1)||
       !init_pcb(&f,3u,0x00010003u,12u,0))
        return fail("source PCB fixtures");
    if(port_v37_sched_chse(&scheduler,f.pcb_va[0],0u)!=PORT_V37_SCHED_OK||
       port_v37_sched_chse(&scheduler,f.pcb_va[1],0u)!=PORT_V37_SCHED_OK||
       port_v37_sched_chse(&scheduler,f.pcb_va[2],0u)!=PORT_V37_SCHED_OK)
        return fail("resident SCH$CHSE enqueue");
    if(get16(f.pcbs[0]+PORT_V37_PCB_STATE)!=PORT_V37_SCH_COM||
       get16(f.pcbs[1]+PORT_V37_PCB_STATE)!=PORT_V37_SCH_COM||
       get16(f.pcbs[2]+PORT_V37_PCB_STATE)!=PORT_V37_SCH_COM||
       f.pcbs[0][PORT_V37_PCB_PRI]!=31u||
       f.pcbs[1][PORT_V37_PCB_PRI]!=15u||
       scheduler.summary[0]!=0x80008000u||scheduler.summary[1]!=0u||
       count(&f,15u)!=2u||count(&f,31u)!=1u||
       get32(f.pcbs[1]+PORT_V37_PCB_SQFL)!=f.pcb_va[2]||
       get32(f.pcbs[2]+PORT_V37_PCB_SQBL)!=f.pcb_va[1]||
       port_v37_sched_check(&scheduler)!=PORT_V37_SCHED_OK)
        return fail("source state, inverted priorities, FIFO and COMQS");
    if(port_v37_sched_chse(&scheduler,f.pcb_va[0],2u)!=PORT_V37_SCHED_OK||
       f.pcbs[0][PORT_V37_PCB_PRI]!=28u||count(&f,31u)!=0u||
       count(&f,28u)!=1u||scheduler.summary[0]!=0x10008000u||
       port_v37_sched_check(&scheduler)!=PORT_V37_SCHED_OK)
        return fail("RSE priority increment and empty-queue bit clear");
    if(port_v37_sched_chse(&scheduler,f.pcb_va[3],0u)!=PORT_V37_SCHED_OK||
       get16(f.pcbs[3]+PORT_V37_PCB_STATE)!=PORT_V37_SCH_COMO||
       scheduler.summary[1]!=(1u<<19)||f.swapper_wakes!=1u||
       port_v37_sched_check(&scheduler)!=PORT_V37_SCHED_OK)
        return fail("nonresident COMO queue and swapper wake callback");
    scheduler.current_priority=15u;
    const unsigned prior_reschedules=f.reschedules;
    if(port_v37_sched_chse(&scheduler,f.pcb_va[1],0u)!=PORT_V37_SCHED_OK||
       f.reschedules!=prior_reschedules+1u||
       port_v37_sched_check(&scheduler)!=PORT_V37_SCHED_OK)
        return fail("resident higher-priority reschedule request");
    uint32_t selected=0u,restore_frame=0u;
    if(port_v37_sched_select(&scheduler,&selected,&restore_frame)!=
           PORT_V37_SCHED_NO_CONTEXT||selected||restore_frame||count(&f,15u)!=2u)
        return fail("scheduler leaves contextless PCB queued");
    for(unsigned i=0;i<3u;i++){
        if(port_v37_pcb_bind_context(f.pcbs[i],sizeof f.pcbs[i],
           f.phd_va[i],f.phd_va[i]+PORT_V37_PHD_PCB)!=PORT_V37_PCB_OK)
            return fail("source PCB PHD/native context binding");
        put32(f.phds[i]+PORT_V37_PHD_KSP,f.frame_va[i]);
        f.frames[i]=(struct port_v37_ia32_restore_frame){
            .gs=0x10u,.fs=0x10u,.es=0x10u,.ds=0x10u,
            .registers={.eip=0x1000u+i*4u,.cs=0x08u,.eflags=2u}};
    }
    f.frames[2].registers.cs=0x08u|3u;
    if(port_v37_sched_select(&scheduler,&selected,&restore_frame)!=
           PORT_V37_SCHED_CORRUPT||selected||restore_frame||
       scheduler.current_pcb||count(&f,15u)!=2u)
        return fail("SCH$SCHED rejects malformed IA-32 context without dequeue");
    f.frames[2].registers.cs=0x08u;
    if(port_v37_sched_select(&scheduler,&selected,&restore_frame)!=
           PORT_V37_SCHED_OK||
       selected!=f.pcb_va[2]||
       restore_frame!=f.frame_va[2]||
       get16(f.pcbs[2]+PORT_V37_PCB_STATE)!=PORT_V37_SCH_CUR||
       scheduler.current_pcb!=selected||scheduler.current_priority!=15u||
       count(&f,15u)!=1u||port_v37_sched_check(&scheduler)!=PORT_V37_SCHED_OK)
        return fail("SCH$SCHED highest-priority FIFO PCB selection");
    const uint32_t saved_pcb1_phd=get32(f.pcbs[1]+PORT_V37_PCB_PHD);
    const uint32_t saved_pcb1_frame=get32(f.pcbs[1]+PORT_V37_PCB_PHYPCB);
    put32(f.pcbs[1]+PORT_V37_PCB_PHD,0u);
    put32(f.pcbs[1]+PORT_V37_PCB_PHYPCB,0u);
    scheduler.native_no_context_count=UINT32_MAX;
    if(port_v37_sched_interrupt(&scheduler,f.frame_va[2],&selected,
           &restore_frame)!=PORT_V37_SCHED_NO_CONTEXT||selected||restore_frame||
       scheduler.current_pcb!=f.pcb_va[2]||
       get16(f.pcbs[2]+PORT_V37_PCB_STATE)!=PORT_V37_SCH_CUR||
       get32(f.pcbs[2]+PORT_V37_PCB_PHYPCB)!=
           f.phd_va[2]+PORT_V37_PHD_PCB||
       count(&f,15u)!=1u||
       scheduler.native_no_context_count!=UINT32_MAX||
       port_v37_sched_check(&scheduler)!=PORT_V37_SCHED_OK)
        return fail("scheduler preserves interrupted current across contextless higher-priority PCB");
    put32(f.pcbs[1]+PORT_V37_PCB_PHD,saved_pcb1_phd);
    put32(f.pcbs[1]+PORT_V37_PCB_PHYPCB,saved_pcb1_frame);
    selected=f.pcb_va[2];
    const uint32_t prior_frame=get32(f.pcbs[2]+PORT_V37_PCB_PHYPCB);
    f.frames[2].registers.cs=0x0bu;
    if(port_v37_sched_save_current(&scheduler,f.frame_va[2])!=
           PORT_V37_SCHED_CORRUPT||scheduler.current_pcb!=selected||
       get32(f.pcbs[2]+PORT_V37_PCB_PHYPCB)!=prior_frame||
       get16(f.pcbs[2]+PORT_V37_PCB_STATE)!=PORT_V37_SCH_CUR)
        return fail("SCH$RESCHED rejects malformed outgoing context atomically");
    f.frames[2].registers.cs=0x08u;
    f.frames[2].registers.eip=0x66666666u;
    put32(f.pcbs[2]+PORT_V37_PCB_SQFL,0x00abcdefu);
    if(port_v37_sched_save_current(&scheduler,f.frame_va[2])!=
           PORT_V37_SCHED_CORRUPT||
       get32(f.pcbs[2]+PORT_V37_PCB_PHYPCB)!=prior_frame||
       get32(f.phds[2]+PORT_V37_PHD_KSP)!=f.frame_va[2]||
       get32(f.phds[2]+PORT_V37_PHD_PC)!=0x1008u||
       scheduler.current_pcb!=selected)
        return fail("SCH$RESCHED rolls context back on queue corruption");
    put32(f.pcbs[2]+PORT_V37_PCB_SQFL,f.pcb_va[2]);
    f.frames[2].registers.eip=0x77777777u;
    if(port_v37_sched_save_current(&scheduler,f.frame_va[2])!=
           PORT_V37_SCHED_OK||
       get32(f.pcbs[2]+PORT_V37_PCB_PHYPCB)!=
           f.phd_va[2]+PORT_V37_PHD_PCB||
       get32(f.phds[2]+PORT_V37_PHD_KSP)!=f.frame_va[2]||
       get32(f.phds[2]+PORT_V37_PHD_PC)!=0x77777777u||
       get16(f.pcbs[2]+PORT_V37_PCB_STATE)!=PORT_V37_SCH_COM||
       count(&f,15u)!=2u||port_v37_sched_check(&scheduler)!=PORT_V37_SCHED_OK)
        return fail("SCH$RESCHED saves native frame, PHD and PCB queue state");
    if(port_v37_sched_select(&scheduler,&selected,&restore_frame)!=
           PORT_V37_SCHED_OK||
       selected!=f.pcb_va[1]||port_v37_sched_check(&scheduler)!=PORT_V37_SCHED_OK)
        return fail("SCH$SCHED equal-priority FIFO selection");
    if(port_v37_sched_resched_current(&scheduler,selected)!=PORT_V37_SCHED_OK||
       port_v37_sched_select(&scheduler,&selected,&restore_frame)!=
           PORT_V37_SCHED_OK||
       selected!=f.pcb_va[2]||port_v37_sched_check(&scheduler)!=PORT_V37_SCHED_OK)
        return fail("SCH$RESCHED round-robin requeue order");
    if(port_v37_sched_select(&scheduler,&selected,&restore_frame)!=
           PORT_V37_SCHED_OK||
       selected!=f.pcb_va[1]||port_v37_sched_check(&scheduler)!=PORT_V37_SCHED_OK)
        return fail("SCH$SCHED equal-priority FIFO selection after requeue");
    if(port_v37_sched_select(&scheduler,&selected,&restore_frame)!=
           PORT_V37_SCHED_OK||
       selected!=f.pcb_va[0]||f.pcbs[0][PORT_V37_PCB_PRI]!=29u||
       scheduler.current_priority!=29u||port_v37_sched_check(&scheduler)!=
           PORT_V37_SCHED_OK)
        return fail("SCH$SCHED source floating priority and selection");
    f.frames[0].registers.eip=0x12345678u;
    if(port_v37_sched_interrupt(&scheduler,f.frame_va[0],&selected,
           &restore_frame)!=PORT_V37_SCHED_OK||selected!=f.pcb_va[0]||
       restore_frame!=f.frame_va[0]||
       get32(f.phds[0]+PORT_V37_PHD_PC)!=0x12345678u||
       get16(f.pcbs[0]+PORT_V37_PCB_STATE)!=PORT_V37_SCH_CUR||
       port_v37_sched_check(&scheduler)!=PORT_V37_SCHED_OK)
        return fail("SCH$RESCHED/SCH$SCHED interrupt-level transaction");
    f.frames[0].registers.cs=0x0bu;
    if(port_v37_sched_interrupt(&scheduler,f.frame_va[0],&selected,
           &restore_frame)!=PORT_V37_SCHED_CORRUPT||selected||restore_frame||
       scheduler.current_pcb!=f.pcb_va[0]||
       get16(f.pcbs[0]+PORT_V37_PCB_STATE)!=PORT_V37_SCH_CUR)
        return fail("interrupt dispatch rejects malformed outgoing frame");
    f.frames[0].registers.cs=0x08u;
    f.frames[0].registers.eip=0x23456789u;
    f.wait_queue.state=PORT_V37_SCH_COM;
    if(port_v37_sched_wait_current(&scheduler,f.wait_queue_va,
           f.frame_va[0],&selected,&restore_frame)!=PORT_V37_SCHED_CORRUPT||
       selected||restore_frame||scheduler.current_pcb!=f.pcb_va[0]||
       f.wait_queue.count||get16(f.pcbs[0]+PORT_V37_PCB_STATE)!=PORT_V37_SCH_CUR)
        return fail("SCH$WAITK rejects a non-wait source process state");
    f.wait_queue.state=PORT_V37_SCH_HIB;
    f.wait_queue.forward=0x00abcdefu;
    if(port_v37_sched_wait_current(&scheduler,f.wait_queue_va,
           f.frame_va[0],&selected,&restore_frame)!=PORT_V37_SCHED_CORRUPT||
       selected||restore_frame||scheduler.current_pcb!=f.pcb_va[0]||
       get16(f.pcbs[0]+PORT_V37_PCB_STATE)!=PORT_V37_SCH_CUR||
       get32(f.phds[0]+PORT_V37_PHD_PC)!=0x12345678u)
        return fail("SCH$WAITK rejects malformed source WQH before mutation");
    f.wait_queue.forward=f.wait_queue.backward=f.wait_queue_va;
    if(port_v37_sched_wait_current(&scheduler,f.wait_queue_va,
           f.frame_va[0],&selected,&restore_frame)!=PORT_V37_SCHED_BLOCKED||
       selected||restore_frame||scheduler.current_pcb||
       f.wait_queue.count!=1u||f.wait_queue.forward!=f.pcb_va[0]||
       f.wait_queue.backward!=f.pcb_va[0]||
       get16(f.pcbs[0]+PORT_V37_PCB_STATE)!=f.wait_queue.state||
       get32(f.phds[0]+PORT_V37_PHD_KSP)!=f.frame_va[0]||
       get32(f.phds[0]+PORT_V37_PHD_PC)!=0x23456789u)
        return fail("SYSWAIT SCH$WAITK saves context and queues source WQH");
    const uint32_t wake_status=port_v37_sched_wake_waiter(&scheduler,
           f.wait_queue_va,f.pcb_va[0],0u);
    if(wake_status!=PORT_V37_SCHED_OK||f.wait_queue.count||
       f.wait_queue.forward!=f.wait_queue_va||
       f.wait_queue.backward!=f.wait_queue_va||
       get16(f.pcbs[0]+PORT_V37_PCB_STATE)!=PORT_V37_SCH_COM||
       f.pcbs[0][PORT_V37_PCB_PRI]!=30u||
       scheduler.summary[0]!=(1u<<30)||scheduler.ready_count[30]!=1u||
       port_v37_sched_check(&scheduler)!=PORT_V37_SCHED_OK){
        fprintf(stderr,"wake status=%u count=%u fwd=%08x back=%08x state=%u summary=%08x check=%u\\n",
            wake_status,f.wait_queue.count,f.wait_queue.forward,
            f.wait_queue.backward,get16(f.pcbs[0]+PORT_V37_PCB_STATE),
            scheduler.summary[0],port_v37_sched_check(&scheduler));
        return fail("SYSWAIT wake removes WQH process and returns it via CHSE");
    }
    /* Keep a nonresident waiter at the WQH head, then block the resident
     * process behind it. Waking the tail must preserve the head and count;
     * waking the nonresident head must route it to COMO and wake the swapper. */
    const unsigned p3_queue=(unsigned)f.pcbs[3][PORT_V37_PCB_PRI]+32u;
    const uint32_t p3_head=0x00100000u+
        p3_queue*(uint32_t)sizeof(struct port_v37_sched_head);
    scheduler.heads[p3_queue].forward=p3_head;
    scheduler.heads[p3_queue].backward=p3_head;
    scheduler.summary[p3_queue>>5]&=~(1u<<(p3_queue&31u));
    put32(f.pcbs[3]+PORT_V37_PCB_SQFL,f.wait_queue_va);
    put32(f.pcbs[3]+PORT_V37_PCB_SQBL,f.wait_queue_va);
    put16(f.pcbs[3]+PORT_V37_PCB_STATE,f.wait_queue.state);
    f.wait_queue.forward=f.wait_queue.backward=f.pcb_va[3];
    f.wait_queue.count=1u;
    if(port_v37_sched_select(&scheduler,&selected,&restore_frame)!=
           PORT_V37_SCHED_OK||selected!=f.pcb_va[0])
        return fail("select resident process before FIFO WQH append");
    if(port_v37_sched_wait_current(&scheduler,f.wait_queue_va,
           f.frame_va[0],&selected,&restore_frame)!=PORT_V37_SCHED_BLOCKED||
       f.wait_queue.count!=2u||f.wait_queue.forward!=f.pcb_va[3]||
       f.wait_queue.backward!=f.pcb_va[0]||
       get32(f.pcbs[3]+PORT_V37_PCB_SQFL)!=f.pcb_va[0]||
       get32(f.pcbs[0]+PORT_V37_PCB_SQBL)!=f.pcb_va[3])
        return fail("SCH$WAITK appends behind existing waiters");
    const uint32_t swaps_before=f.swapper_wakes;
    const uint32_t wake_tail=port_v37_sched_wake_waiter(&scheduler,
        f.wait_queue_va,f.pcb_va[0],1u);
    const uint32_t check_after_tail=port_v37_sched_check(&scheduler);
    if(wake_tail!=PORT_V37_SCHED_OK||f.wait_queue.count!=1u||
       f.wait_queue.forward!=f.pcb_va[3]||f.wait_queue.backward!=f.pcb_va[3]||
       get16(f.pcbs[0]+PORT_V37_PCB_STATE)!=PORT_V37_SCH_COM||
       check_after_tail!=PORT_V37_SCHED_OK){
        fprintf(stderr,"tail=%u count=%u state0=%u pri0=%u q29=%u ready29=%u summary=%08x/%08x links=%08x/%08x check=%u\\n",
            wake_tail,f.wait_queue.count,get16(f.pcbs[0]+PORT_V37_PCB_STATE),
            f.pcbs[0][PORT_V37_PCB_PRI],count(&f,29u),
            scheduler.ready_count[29],scheduler.summary[0],scheduler.summary[1],
            get32(f.pcbs[0]+PORT_V37_PCB_SQBL),
            get32(f.pcbs[0]+PORT_V37_PCB_SQFL),check_after_tail);
        return fail("RSE wake unlinks tail and preserves WQH head");
    }
    const uint32_t wake_nonresident=port_v37_sched_wake_waiter(&scheduler,
        f.wait_queue_va,f.pcb_va[3],0u);
    if(wake_nonresident!=PORT_V37_SCHED_OK||f.wait_queue.count||
       f.wait_queue.forward!=f.wait_queue_va||
       f.wait_queue.backward!=f.wait_queue_va||
       get16(f.pcbs[3]+PORT_V37_PCB_STATE)!=PORT_V37_SCH_COMO||
       f.swapper_wakes!=swaps_before+1u||
       port_v37_sched_check(&scheduler)!=PORT_V37_SCHED_OK){
        fprintf(stderr,"tail=%u nonres=%u count=%u fwd=%08x back=%08x state=%u swaps=%u p3links=%08x/%08x pri=%u/%u check=%u\\n",
            wake_tail,wake_nonresident,f.wait_queue.count,
            f.wait_queue.forward,f.wait_queue.backward,
            get16(f.pcbs[3]+PORT_V37_PCB_STATE),f.swapper_wakes,
            get32(f.pcbs[3]+PORT_V37_PCB_SQFL),
            get32(f.pcbs[3]+PORT_V37_PCB_SQBL),
            f.pcbs[3][PORT_V37_PCB_PRI],f.pcbs[3][PORT_V37_PCB_PRIB],
            port_v37_sched_check(&scheduler));
        return fail("RSE wake preserves WQH FIFO and routes nonresident PCB");
    }
    f.wait_queue.state=3u; /* SCH$C_CEF */
    if(port_v37_sched_select(&scheduler,&selected,&restore_frame)!=
           PORT_V37_SCHED_OK||selected!=f.pcb_va[0]||
       port_v37_sched_wait_current(&scheduler,f.wait_queue_va,
           f.frame_va[0],&selected,&restore_frame)!=PORT_V37_SCHED_BLOCKED||
       port_v37_sched_rse_event(&scheduler,PORT_V37_RSE_PFCOM,
           f.wait_queue_va,f.pcb_va[0],0u)!=PORT_V37_SCHED_IGNORED||
       f.wait_queue.count!=1u||
       port_v37_sched_rse_event(&scheduler,PORT_V37_RSE_EVENT_CEF,
           f.wait_queue_va,f.pcb_va[0],0u)!=PORT_V37_SCHED_OK||
       f.wait_queue.count||
       get16(f.pcbs[0]+PORT_V37_PCB_STATE)!=PORT_V37_SCH_COM||
       get32(f.phds[0]+PORT_V37_PHD_PC)!=f.frames[0].registers.eip||
       get32(f.phds[0]+PORT_V37_PHD_R0)!=1u||
       f.frames[0].registers.eax!=1u||
       port_v37_sched_check(&scheduler)!=PORT_V37_SCHED_OK)
        return fail("RSE CEF state mask, saved syscall completion and CHSE transition");
    f.wait_queue.state=5u; /* SCH$C_LEF */
    f.frames[0].registers.eip=0x34567890u;
    f.frames[0].registers.eax=0xfeedc0deu;
    if(port_v37_sched_select(&scheduler,&selected,&restore_frame)!=
           PORT_V37_SCHED_OK||selected!=f.pcb_va[0]||
       port_v37_sched_wait_current(&scheduler,f.wait_queue_va,
           f.frame_va[0],&selected,&restore_frame)!=PORT_V37_SCHED_BLOCKED)
        return fail("SYS$WFLOR reaches source LEF wait state");
    const uint32_t lef_pc=get32(f.phds[0]+PORT_V37_PHD_PC);
    const uint32_t lef_r0=get32(f.phds[0]+PORT_V37_PHD_R0);
    const uint32_t lef_eax=f.frames[0].registers.eax;
    const uint16_t lef_cs=f.frames[0].registers.cs;
    f.frames[0].registers.cs=0x3bu; /* User code selector with kernel data selectors. */
    if(port_v37_sched_rse_event(&scheduler,PORT_V37_RSE_EVENT_LEF,
           f.wait_queue_va,f.pcb_va[0],0u)!=PORT_V37_SCHED_CORRUPT||
       get32(f.phds[0]+PORT_V37_PHD_PC)!=lef_pc||
       get32(f.phds[0]+PORT_V37_PHD_R0)!=lef_r0||
       f.frames[0].registers.eax!=lef_eax||
       get16(f.pcbs[0]+PORT_V37_PCB_STATE)!=PORT_V37_SCH_LEF||
       f.wait_queue.count!=1u)
        return fail("RSE LEF rejects malformed native IRET context before mutation");
    f.frames[0].registers.cs=lef_cs;
    f.wait_queue.backward=0x00abcdefu;
    if(port_v37_sched_rse_event(&scheduler,PORT_V37_RSE_EVENT_LEF,
           f.wait_queue_va,f.pcb_va[0],0u)!=PORT_V37_SCHED_CORRUPT||
       get32(f.phds[0]+PORT_V37_PHD_PC)!=lef_pc||
       get32(f.phds[0]+PORT_V37_PHD_R0)!=1u||scheduler.current_pcb||
       f.frames[0].registers.eax!=lef_eax||
       get16(f.pcbs[0]+PORT_V37_PCB_STATE)!=5u)
        return fail("RSE LEF rolls back saved result on corrupt WQH");
    f.wait_queue.backward=f.pcb_va[0];
    if(port_v37_sched_rse_event(&scheduler,PORT_V37_RSE_EVENT_LEF,
           f.wait_queue_va,f.pcb_va[0],0u)!=PORT_V37_SCHED_OK||
       get32(f.phds[0]+PORT_V37_PHD_PC)!=lef_pc||
       get32(f.phds[0]+PORT_V37_PHD_R0)!=1u||f.wait_queue.count||
       f.frames[0].registers.eax!=1u||
       port_v37_sched_check(&scheduler)!=PORT_V37_SCHED_OK)
        return fail("RSE LEF commits saved syscall result after queue validation");
    f.wait_queue.state=PORT_V37_SCH_MWAIT;
    f.frames[0].registers.eip=0x45678901u;
    if(port_v37_sched_select(&scheduler,&selected,&restore_frame)!=
           PORT_V37_SCHED_OK||selected!=f.pcb_va[0]||
       port_v37_sched_wait_current(&scheduler,f.wait_queue_va,
           f.frame_va[0],&selected,&restore_frame)!=PORT_V37_SCHED_BLOCKED||
       port_v37_sched_rse_event(&scheduler,PORT_V37_RSE_AST,
           f.wait_queue_va,f.pcb_va[0],0u)!=PORT_V37_SCHED_OK||
       f.wait_queue.count||
       get16(f.pcbs[0]+PORT_V37_PCB_STATE)!=PORT_V37_SCH_COM||
       get32(f.phds[0]+PORT_V37_PHD_PC)!=0x45678901u||
       port_v37_sched_check(&scheduler)!=PORT_V37_SCHED_OK)
        return fail("RSE AST wakes MWAIT process without altering its return frame");
    const uint8_t old_priority=f.pcbs[0][PORT_V37_PCB_PRI];
    if(port_v37_sched_rse_event(&scheduler,PORT_V37_RSE_SETPRI,0u,
           f.pcb_va[0],32u)!=PORT_V37_SCHED_BADPARAM||
       f.pcbs[0][PORT_V37_PCB_PRI]!=old_priority||
       port_v37_sched_rse_event(&scheduler,PORT_V37_RSE_SETPRI,0u,
           f.pcb_va[0],20u)!=PORT_V37_SCHED_OK||
       f.pcbs[0][PORT_V37_PCB_PRI]!=20u||
       get16(f.pcbs[0]+PORT_V37_PCB_STATE)!=PORT_V37_SCH_COM||
       count(&f,20u)!=1u||port_v37_sched_check(&scheduler)!=PORT_V37_SCHED_OK)
        return fail("RSE SETPRI precomputed priority COM queue transition");
    const unsigned swaps_before_setpri=f.swapper_wakes;
    if(port_v37_sched_rse_event(&scheduler,PORT_V37_RSE_SETPRI,0u,
           f.pcb_va[3],17u)!=PORT_V37_SCHED_OK||
       f.pcbs[3][PORT_V37_PCB_PRI]!=17u||
       get16(f.pcbs[3]+PORT_V37_PCB_STATE)!=PORT_V37_SCH_COMO||
       f.swapper_wakes!=swaps_before_setpri+1u||count(&f,49u)!=1u||
       port_v37_sched_check(&scheduler)!=PORT_V37_SCHED_OK)
        return fail("RSE SETPRI nonresident COMO and swapper wake transition");
    struct port_v37_scheduler start_scheduler;
    struct fixture start={.scheduler=&start_scheduler,
        .pcb_va={0x00800000u,0x00801000u,0x00802000u,0x00803000u},
        .frame_va={0x00900000u,0x00901000u,0x00902000u,0x00903000u},
        .phd_va={0x00a00000u,0x00a01000u,0x00a02000u,0x00a03000u},
        .wait_queue_va=0x00b00000u,.live_cr3=0x00d00000u};
    start.wait_queue.forward=start.wait_queue.backward=start.wait_queue_va;
    const struct port_v37_sched_ops start_ops={
        .context=&start,.address=addr,.resolve=resolve,
        .wake_swapper=wake,.request_reschedule=reschedule,
        .native_cr3_by_pix=start.cr3_by_pix,.native_cr3_slots=16u,
        .current_cr3=current_cr3};
    if(port_v37_sched_init(&start_scheduler,&start_ops)!=PORT_V37_SCHED_OK)
        return fail("initial process activation scheduler setup");
    const struct port_v37_pcb_seed start_pcb={.address=start.pcb_va[0],
        .pid=0x00010008u,.priority=16u,.name="SYSTEM",.name_bytes=6u};
    if(port_v37_pcb_init_genpcb(start.pcbs[0],sizeof start.pcbs[0],
           &start_pcb)!=PORT_V37_PCB_OK)
        return fail("initial process activation PCB template");
    start.frames[0]=(struct port_v37_ia32_restore_frame){
        .gs=0x10u,.fs=0x10u,.es=0x10u,.ds=0x10u,
        .registers={.cs=0x08u,.eflags=0x202u,.eip=0x00123456u,
                    .user_ss=start.pcb_va[0]}};
    const struct port_v37_phd_seed start_phd={.entry_pc=0x00123456u,
        .initial_r4=start.pcb_va[0],
        .p0_page_table=0x00c00000u,.p0_limit=12u,
        .p1_page_table=0x00c01000u,.p1_limit=0x200000u,
        .first_free_p1_va=0x7ffffe00u,.priority=0u,.ast_level=4u};
    struct port_v37_phd_seed bad_start_phd=start_phd;
    bad_start_phd.p0_page_table|=1u;
    if(port_v37_sched_start_context(&start_scheduler,start.pcb_va[0],
           start.phd_va[0],start.phd_va[0]+PORT_V37_PHD_PCB,
           start.frame_va[0],&bad_start_phd,0u)!=
           PORT_V37_SCHED_BADPARAM||
       get32(start.pcbs[0]+PORT_V37_PCB_PHD)||
       get32(start.pcbs[0]+PORT_V37_PCB_PHYPCB)||
       port_v37_sched_check(&start_scheduler)!=PORT_V37_SCHED_OK)
        return fail("invalid PHD mapping rejected before INIT context commit");
    start.frames[0].registers.user_ss++;
    if(port_v37_sched_start_context(&start_scheduler,start.pcb_va[0],
           start.phd_va[0],start.phd_va[0]+PORT_V37_PHD_PCB,
           start.frame_va[0],&start_phd,0u)!=PORT_V37_SCHED_CORRUPT||
       get32(start.pcbs[0]+PORT_V37_PCB_PHD)||
       get32(start.pcbs[0]+PORT_V37_PCB_PHYPCB)||
       port_v37_sched_check(&start_scheduler)!=PORT_V37_SCHED_OK)
        return fail("native entry stack argument must preserve source PHD R4");
    start.frames[0].registers.user_ss--;
    if(port_v37_sched_start_context(&start_scheduler,start.pcb_va[0],
           start.phd_va[0],start.phd_va[0]+PORT_V37_PHD_PCB,
           start.frame_va[0],&start_phd,0u)!=
           PORT_V37_SCHED_OK||
       get32(start.pcbs[0]+PORT_V37_PCB_PHD)!=start.phd_va[0]||
       get32(start.pcbs[0]+PORT_V37_PCB_PHYPCB)!=
           start.phd_va[0]+PORT_V37_PHD_PCB||
       get32(start.phds[0]+PORT_V37_PHD_KSP)!=start.frame_va[0]||
       get32(start.phds[0]+PORT_V37_PHD_PC)!=start.frames[0].registers.eip||
       port_v37_sched_select(&start_scheduler,&selected,&restore_frame)!=
           PORT_V37_SCHED_OK||selected!=start.pcb_va[0]||
       restore_frame!=start.frame_va[0]||
       start_scheduler.current_cr3!=start.live_cr3||
       start.cr3_by_pix[8u]!=start.live_cr3||
       port_v37_sched_check(&start_scheduler)!=PORT_V37_SCHED_OK)
        return fail("INIT PHD/native-frame binding and executable CHSE activation");
    const struct port_v37_pcb_seed quota_pcb={.address=start.pcb_va[1],
        .pid=0x00010009u,.priority=20u,.name="USER",.name_bytes=4u};
    if(port_v37_pcb_init_genpcb(start.pcbs[1],sizeof start.pcbs[1],
           &quota_pcb)!=PORT_V37_PCB_OK)
        return fail("PROCSTRT quota PCB template");
    start.frames[1]=(struct port_v37_ia32_restore_frame){
        .gs=0x10u,.fs=0x10u,.es=0x10u,.ds=0x10u,
        .registers={.cs=0x08u,.eflags=0x202u,.eip=0x00123456u,
                    .user_ss=start.pcb_va[1]}};
    struct port_v37_phd_seed quota_seed=start_phd;
    quota_seed.initial_r4=start.pcb_va[1];
    quota_seed.native_cr3=0x00e00000u;
    const struct port_v37_phd_process_quotas process_quotas={
        .cpu_limit=0x10203040u,.ast_limit=37u,
        .privileges=UINT64_C(0x0000000000400080),.base_priority=0u};
    const struct port_v37_phd_ws_quotas ws_quotas={
        .physical_pages=300u,.free_page_limit=20u,.max_working_set=256u,
        .extent=300u,.quota=150u,.default_size=100u};
    if(port_v37_sched_start_context_quotas(&start_scheduler,
           start.pcb_va[1],start.phd_va[1],
           start.phd_va[1]+PORT_V37_PHD_PCB,start.frame_va[1],&quota_seed,
           64u,&process_quotas,&ws_quotas,0u)!=PORT_V37_SCHED_OK||
       get32(start.phds[1]+PORT_V37_PHD_CPULIM)!=process_quotas.cpu_limit||
       get16(start.phds[1]+PORT_V37_PHD_ASTLM)!=process_quotas.ast_limit||
       start.phds[1][PORT_V37_PHD_AUTHPRI]!=
           start.pcbs[1][PORT_V37_PCB_PRIB]||
       get64(start.phds[1]+PORT_V37_PHD_PRIVMSK)!=process_quotas.privileges||
       get64(start.phds[1]+PORT_V37_PHD_AUTHPRIV)!=process_quotas.privileges||
       get16(start.phds[1]+PORT_V37_PHD_WSEXTENT)!=319u||
       get16(start.phds[1]+PORT_V37_PHD_WSQUOTA)!=213u||
       get16(start.phds[1]+PORT_V37_PHD_DFWSCNT)!=163u||
       get32(start.pcbs[1]+PORT_V37_PCB_PHD)!=start.phd_va[1]||
       port_v37_sched_check(&start_scheduler)!=PORT_V37_SCHED_OK)
        return fail("PROCSTRT quotas and privilege state precede executable CHSE");
    if(port_v37_sched_resched_current(&start_scheduler,start.pcb_va[0])!=
           PORT_V37_SCHED_OK||
       port_v37_sched_select(&start_scheduler,&selected,&restore_frame)!=
           PORT_V37_SCHED_OK||selected!=start.pcb_va[1]||
       restore_frame!=start.frame_va[1]||
       start_scheduler.current_cr3!=quota_seed.native_cr3||
       start.cr3_by_pix[9u]!=quota_seed.native_cr3||
       port_v37_sched_check(&start_scheduler)!=PORT_V37_SCHED_OK)
        return fail("SCH$SCHED selects process-specific native CR3 by source PIX");
    const struct port_v37_pcb_seed rejected_pcb={.address=start.pcb_va[2],
        .pid=0x0001000au,.priority=8u,.name="BAD",.name_bytes=3u};
    if(port_v37_pcb_init_genpcb(start.pcbs[2],sizeof start.pcbs[2],
           &rejected_pcb)!=PORT_V37_PCB_OK)
        return fail("invalid quota activation PCB template");
    start.frames[2]=(struct port_v37_ia32_restore_frame){
        .gs=0x10u,.fs=0x10u,.es=0x10u,.ds=0x10u,
        .registers={.cs=0x08u,.eflags=0x202u,.eip=0x00123456u,
                    .user_ss=start.pcb_va[2]}};
    struct port_v37_phd_ws_quotas invalid_ws=ws_quotas;
    invalid_ws.physical_pages=10u;invalid_ws.free_page_limit=20u;
    struct port_v37_phd_seed rejected_seed=quota_seed;
    rejected_seed.initial_r4=start.pcb_va[2];
    if(port_v37_sched_start_context_quotas(&start_scheduler,
           start.pcb_va[2],start.phd_va[2],
           start.phd_va[2]+PORT_V37_PHD_PCB,start.frame_va[2],&rejected_seed,
           64u,&process_quotas,&invalid_ws,0u)!=PORT_V37_SCHED_BADPARAM||
       get32(start.pcbs[2]+PORT_V37_PCB_PHD)||
       get32(start.pcbs[2]+PORT_V37_PCB_PHYPCB)||
       memcmp(start.phds[2],(uint8_t[PORT_V37_PHD_BYTES]){0},
              PORT_V37_PHD_BYTES)||
       port_v37_sched_check(&start_scheduler)!=PORT_V37_SCHED_OK)
        return fail("invalid PROCSTRT quotas leave PCB/PHD/scheduler unpublished");
    puts("SYSWAIT/SCHED/RSE plus transactional PROCSTRT PHD quota activation PASS");
    return 0;
}
