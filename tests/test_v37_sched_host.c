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
    unsigned reschedules,swapper_wakes;
};
static uint32_t addr(void *context,const void *p){
    struct fixture *f=context;
    uintptr_t begin=(uintptr_t)&f->scheduler->heads[0];
    uintptr_t end=(uintptr_t)&f->scheduler->heads[64];
    uintptr_t value=(uintptr_t)p;
    if(value>=begin&&value<end)return 0x00100000u+(uint32_t)(value-begin);
    for(unsigned i=0;i<4u;i++)if(p==f->pcbs[i])return f->pcb_va[i];
    if(p==&f->wait_queue)return f->wait_queue_va;
    return 0u;
}
static uint8_t *resolve(void *context,uint32_t va,size_t bytes){
    struct fixture *f=context;
    const uint32_t hb=0x00100000u;
    const uint32_t he=hb+64u*sizeof(struct port_v37_sched_head);
    if(va>=hb&&va<=he&&bytes<=he-va)
        return (uint8_t *)&f->scheduler->heads[0]+(va-hb);
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
    const struct port_v37_sched_ops ops={&f,addr,resolve,wake,reschedule};
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
           f.phd_va[i],f.frame_va[i])!=PORT_V37_PCB_OK)
            return fail("source PCB PHD/native context binding");
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
       get32(f.phds[2]+PORT_V37_PHD_KSP)!=0u||
       get32(f.phds[2]+PORT_V37_PHD_PC)!=0u||
       scheduler.current_pcb!=selected)
        return fail("SCH$RESCHED rolls context back on queue corruption");
    put32(f.pcbs[2]+PORT_V37_PCB_SQFL,f.pcb_va[2]);
    f.frames[2].registers.eip=0x77777777u;
    if(port_v37_sched_save_current(&scheduler,f.frame_va[2])!=
           PORT_V37_SCHED_OK||
       get32(f.pcbs[2]+PORT_V37_PCB_PHYPCB)!=f.frame_va[2]||
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
    const uint32_t lef_eax=f.frames[0].registers.eax;
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
    puts("SYSWAIT SCH$WAITK/WQH wake plus SCHED SCH$RESCHED/SCH$SCHED and RSE SCH$CHSE PASS");
    return 0;
}
