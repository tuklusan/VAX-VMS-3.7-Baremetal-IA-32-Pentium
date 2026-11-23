#include "vms_v37_sched.h"
#include <stdio.h>
#include <string.h>

struct fixture {
    struct port_v37_scheduler *scheduler;
    uint8_t pcbs[4][PORT_V37_PCB_BYTES];
    uint32_t pcb_va[4];
    unsigned reschedules,swapper_wakes;
};
static uint32_t addr(void *context,const void *p){
    struct fixture *f=context;
    uintptr_t begin=(uintptr_t)&f->scheduler->heads[0];
    uintptr_t end=(uintptr_t)&f->scheduler->heads[64];
    uintptr_t value=(uintptr_t)p;
    if(value>=begin&&value<end)return 0x00100000u+(uint32_t)(value-begin);
    for(unsigned i=0;i<4u;i++)if(p==f->pcbs[i])return f->pcb_va[i];
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
        .pid=pid,.priority=priority,.name="PROC",.name_bytes=4u};
    if(port_v37_pcb_init_genpcb(f->pcbs[slot],sizeof f->pcbs[slot],&seed)!=
       PORT_V37_PCB_OK)return 0u;
    if(!resident)memset(f->pcbs[slot]+PORT_V37_PCB_STS,0,4u);
    return PORT_V37_PCB_OK;
}
static uint16_t get16(const uint8_t *p){return (uint16_t)p[0]|((uint16_t)p[1]<<8);}
static uint32_t get32(const uint8_t *p){
    return (uint32_t)p[0]|((uint32_t)p[1]<<8)|
       ((uint32_t)p[2]<<16)|((uint32_t)p[3]<<24);
}
static void put32(uint8_t *p,uint32_t x){
    p[0]=(uint8_t)x;p[1]=(uint8_t)(x>>8);
    p[2]=(uint8_t)(x>>16);p[3]=(uint8_t)(x>>24);
}

int main(void){
    struct port_v37_scheduler scheduler;struct fixture f={
        .scheduler=&scheduler,.pcb_va={0x00200000u,0x00201000u,
                                       0x00202000u,0x00203000u}};
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
    uint32_t selected=0u;
    if(port_v37_sched_select(&scheduler,&selected)!=PORT_V37_SCHED_NO_CONTEXT||
       selected||count(&f,15u)!=2u)
        return fail("scheduler leaves contextless PCB queued");
    for(unsigned i=0;i<3u;i++){
        put32(f.pcbs[i]+PORT_V37_PCB_PHD,0x00300000u+i*0x1000u);
        put32(f.pcbs[i]+PORT_V37_PCB_PHYPCB,0x00400000u+i*0x1000u);
    }
    if(port_v37_sched_select(&scheduler,&selected)!=PORT_V37_SCHED_OK||
       selected!=f.pcb_va[2]||
       get16(f.pcbs[2]+PORT_V37_PCB_STATE)!=PORT_V37_SCH_CUR||
       scheduler.current_pcb!=selected||scheduler.current_priority!=15u||
       count(&f,15u)!=1u||port_v37_sched_check(&scheduler)!=PORT_V37_SCHED_OK)
        return fail("SCH$SCHED highest-priority FIFO PCB selection");
    if(port_v37_sched_select(&scheduler,&selected)!=PORT_V37_SCHED_OK||
       selected!=f.pcb_va[1]||port_v37_sched_check(&scheduler)!=PORT_V37_SCHED_OK)
        return fail("SCH$SCHED equal-priority FIFO selection");
    if(port_v37_sched_select(&scheduler,&selected)!=PORT_V37_SCHED_OK||
       selected!=f.pcb_va[0]||f.pcbs[0][PORT_V37_PCB_PRI]!=29u||
       scheduler.current_priority!=29u||port_v37_sched_check(&scheduler)!=
           PORT_V37_SCHED_OK)
        return fail("SCH$SCHED source floating priority and selection");
    if(port_v37_sched_select(&scheduler,&selected)!=PORT_V37_SCHED_EMPTY||
       selected)
        return fail("SCH$SCHED empty resident set");
    puts("SYS/SRC/SCHED.MAR SCH$SCHED selection and RSE.MAR SCH$CHSE queue rules PASS");
    return 0;
}
