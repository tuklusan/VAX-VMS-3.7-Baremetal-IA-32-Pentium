#include "vms_v37_sched.h"
#include "vms_v37_pcb.h"
#include "vms_v37_phd.h"
#include "vms_v37_dyn_tags.h"
#include <stdio.h>
#include <string.h>

struct fixture {
    uint8_t pcb[PORT_V37_PCB_BYTES];
    uint8_t phd[PORT_V37_PHD_BYTES];
    uint8_t ceb[PORT_V37_CEB_BYTES];
    uint8_t acb[5][136];
    struct port_v37_scheduler scheduler;
    unsigned arrivals,current_levels;
    uint8_t last_level;
    unsigned sync_enters,sync_leaves;
};

static uint32_t address(void *context,const void *pointer){
    struct fixture *f=context;
    const uintptr_t p=(uintptr_t)pointer;
    const uintptr_t first=(uintptr_t)&f->scheduler;
    const uintptr_t end=first+sizeof f->scheduler;
    if(p>=first&&p<end)return 0x5000u+(uint32_t)(p-first);
    if(p==(uintptr_t)f->ceb)return 0x4000u;
    return 0u;
}

static uint8_t *resolve(void *context,uint32_t va,size_t bytes){
    struct fixture *f=context;
    if(va>=0x1000u&&va<=0x1000u+sizeof f->pcb&&
       bytes<=0x1000u+sizeof f->pcb-va)
        return f->pcb+(va-0x1000u);
    if(va>=0x2000u&&va<=0x2000u+sizeof f->phd&&
       bytes<=0x2000u+sizeof f->phd-va)
        return f->phd+(va-0x2000u);
    if(va>=0x3000u&&va<0x3500u){
        const unsigned index=(va-0x3000u)/0x100u;
        const unsigned offset=(va-0x3000u)%0x100u;
        if(index<5u&&offset<=sizeof f->acb[index]&&
           bytes<=sizeof f->acb[index]-offset)
            return f->acb[index]+offset;
    }
    if(va>=0x4000u&&va<=0x4000u+sizeof f->ceb&&
       bytes<=0x4000u+sizeof f->ceb-va)
        return f->ceb+(va-0x4000u);
    if(va>=0x5000u&&va<=0x5000u+sizeof f->scheduler&&
       bytes<=0x5000u+sizeof f->scheduler-va)
        return (uint8_t *)&f->scheduler+(va-0x5000u);
    return 0;
}

static uint32_t enter_sync(void *context){
    struct fixture *f=context;f->sync_enters++;return 0x55u;
}
static void leave_sync(void *context,uint32_t token){
    struct fixture *f=context;if(token==0x55u)f->sync_leaves++;
}
static void ast_arrival(void *context,uint32_t pcb){
    struct fixture *f=context;if(pcb==0x1000u)f->arrivals++;
}
static void set_ast_level(void *context,uint8_t level){
    struct fixture *f=context;f->current_levels++;f->last_level=level;
}
static void put16(uint8_t *p,uint16_t x){p[0]=(uint8_t)x;p[1]=(uint8_t)(x>>8);}
static void put32(uint8_t *p,uint32_t x){
    p[0]=(uint8_t)x;p[1]=(uint8_t)(x>>8);
    p[2]=(uint8_t)(x>>16);p[3]=(uint8_t)(x>>24);
}
static uint32_t get32(const uint8_t *p){
    return (uint32_t)p[0]|((uint32_t)p[1]<<8)|
        ((uint32_t)p[2]<<16)|((uint32_t)p[3]<<24);
}
static void make_acb(struct fixture *f,unsigned index,uint8_t rmod,
                     uint32_t pid){
    memset(f->acb[index],0,sizeof f->acb[index]);
    put16(f->acb[index]+8u,sizeof f->acb[index]);
    f->acb[index][10]=PORT_V37_DYN_IRP;
    f->acb[index][11]=rmod;
    put32(f->acb[index]+12u,pid);
}
static int fail(const char *what){fprintf(stderr,"QAST FAIL: %s\n",what);return 1;}

static int auto_wait_state(unsigned state){
    struct fixture f={0};
    const struct port_v37_pcb_seed seed={.address=0x1000u,
        .pid=0x00010001u,.phd_address=0x2000u,.priority=8u,
        .name="WAKE",.name_bytes=4u};
    if(port_v37_pcb_init_genpcb(f.pcb,sizeof f.pcb,&seed)!=PORT_V37_PCB_OK)
        return 0;
    const struct port_v37_sched_ops ops={.context=&f,.address=address,
        .resolve=resolve,.enter_synchronization=enter_sync,
        .leave_synchronization=leave_sync,.ast_arrival=ast_arrival};
    if(port_v37_sched_init(&f.scheduler,&ops)!=PORT_V37_SCHED_OK)return 0;
    struct port_v37_wait_queue *wait=&f.scheduler.wait_headers[state];
    uint32_t wait_va=address(&f,wait);
    if(state==PORT_V37_SCH_CEF){
        put16(f.ceb+8u,PORT_V37_CEB_BYTES);
        f.ceb[PORT_V37_CEB_TYPE]=PORT_V37_CEB_NORMAL;
        put32(f.ceb+PORT_V37_CEB_WQH,0x1000u);
        put32(f.ceb+PORT_V37_CEB_WQH+4u,0x1000u);
        put16(f.ceb+PORT_V37_CEB_WQH+8u,1u);
        put16(f.ceb+PORT_V37_CEB_WQH+10u,PORT_V37_SCH_CEF);
        put32(f.pcb+PORT_V37_PCB_EFC2P,0x4000u);
        f.pcb[PORT_V37_PCB_WEFC]=2u;
        wait_va=0x4000u+PORT_V37_CEB_WQH;
    }else{
        wait->forward=wait->backward=0x1000u;
        wait->count=1u;
    }
    put32(f.pcb+PORT_V37_PCB_SQFL,wait_va);
    put32(f.pcb+PORT_V37_PCB_SQBL,wait_va);
    put16(f.pcb+PORT_V37_PCB_STATE,(uint16_t)state);
    make_acb(&f,0u,1u,seed.pid);
    const uint32_t result=port_v37_sched_queue_ast_rse(&f.scheduler,
        0x1000u,0x3000u,0u,1u);
    const uint8_t *selected_wait=state==PORT_V37_SCH_CEF?
        f.ceb+PORT_V37_CEB_WQH:(const uint8_t *)wait;
    if(result!=PORT_V37_SCHED_OK||
       ((uint16_t)(selected_wait[8]|((uint16_t)selected_wait[9]<<8)))||
       get32(selected_wait)!=wait_va||get32(selected_wait+4u)!=wait_va||
       f.pcb[PORT_V37_PCB_STATE]!=PORT_V37_SCH_COM||
       f.arrivals!=1u||f.sync_enters!=1u||f.sync_leaves!=1u||
       port_v37_sched_check(&f.scheduler)!=PORT_V37_SCHED_OK){
        fprintf(stderr,"state=%u result=%u wqh=%08x/%08x count=%u pcbstate=%u "
            "arrivals=%u sync=%u/%u check=%u\n",state,result,
            get32(selected_wait),get32(selected_wait+4u),
            (unsigned)(selected_wait[8]|((uint16_t)selected_wait[9]<<8)),
            f.pcb[PORT_V37_PCB_STATE],f.arrivals,f.sync_enters,f.sync_leaves,
            port_v37_sched_check(&f.scheduler));
        return 0;
    }
    return 1;
}

int main(void){
    struct fixture f={0};
    const struct port_v37_pcb_seed seed={.address=0x1000u,
        .pid=0x00010001u,.phd_address=0x2000u,.priority=8u,
        .name="QAST",.name_bytes=4u};
    if(port_v37_pcb_init_genpcb(f.pcb,sizeof f.pcb,&seed)!=PORT_V37_PCB_OK)
        return fail("source PCB fixture");
    const struct port_v37_sched_ops ops={.context=&f,.address=address,
        .resolve=resolve,.enter_synchronization=enter_sync,
        .leave_synchronization=leave_sync,.ast_arrival=ast_arrival,
        .set_current_ast_level=set_ast_level};
    if(port_v37_sched_init(&f.scheduler,&ops)!=PORT_V37_SCHED_OK)
        return fail("scheduler fixture");
    make_acb(&f,0u,3u,seed.pid); /* user mode arrives first */
    make_acb(&f,1u,1u,seed.pid); /* supervisor moves before user */
    make_acb(&f,2u,0x80u,seed.pid); /* KAST precedes every user AST */
    make_acb(&f,3u,0x80u,seed.pid); /* equal KASTs retain FIFO order */
    if(port_v37_sched_queue_ast(&f.scheduler,0x1000u,0x3000u)!=
           PORT_V37_SCHED_OK||
       port_v37_sched_queue_ast(&f.scheduler,0x1000u,0x3100u)!=
           PORT_V37_SCHED_OK||f.phd[PORT_V37_PHD_ASTLVL]!=1u||
       f.arrivals!=2u)
        return fail("ascending access-mode order and PHD ASTLVL");
    f.scheduler.current_pcb=0x1000u;
    if(port_v37_sched_queue_ast(&f.scheduler,0x1000u,0x3200u)!=
           PORT_V37_SCHED_OK||
       port_v37_sched_queue_ast(&f.scheduler,0x1000u,0x3300u)!=
           PORT_V37_SCHED_OK)
        return fail("ordered source AST queue insertion");
    if(get32(f.pcb+PORT_V37_PCB_ASTQFL)!=0x3200u||
       get32(f.acb[2])!=0x3300u||get32(f.acb[3])!=0x3100u||
       get32(f.acb[1])!=0x3000u||
       get32(f.pcb+PORT_V37_PCB_ASTQBL)!=0x3000u||
       f.phd[PORT_V37_PHD_ASTLVL]!=0u||f.arrivals!=2u||
       f.sync_enters!=4u||f.sync_leaves!=4u||f.current_levels!=2u||
       f.last_level!=0u)
        return fail("KAST/mode order, ASTLVL and arrival notification");

    uint8_t pcb_links[8],tail_links[8],candidate_links[8];
    memcpy(pcb_links,f.pcb+PORT_V37_PCB_ASTQFL,sizeof pcb_links);
    memcpy(tail_links,f.acb[0],sizeof tail_links);
    memcpy(candidate_links,f.acb[4],sizeof candidate_links);
    make_acb(&f,4u,2u,0x00010002u);
    memcpy(candidate_links,f.acb[4],sizeof candidate_links);
    const uint32_t rejected=port_v37_sched_queue_ast(&f.scheduler,
        0x1000u,0x3400u);
    if(rejected!=PORT_V37_SCHED_CORRUPT||
       memcmp(pcb_links,f.pcb+PORT_V37_PCB_ASTQFL,sizeof pcb_links)||
       memcmp(tail_links,f.acb[0],sizeof tail_links)||
       memcmp(candidate_links,f.acb[4],sizeof candidate_links)||
       f.arrivals!=2u||f.current_levels!=2u)
        return fail("PID mismatch rejected without changing queues");

    make_acb(&f,4u,2u,seed.pid);
    f.acb[4][10]=PORT_V37_DYN_UCB;
    memcpy(candidate_links,f.acb[4],sizeof candidate_links);
    const uint32_t wrong_type=port_v37_sched_queue_ast(&f.scheduler,
        0x1000u,0x3400u);
    if(wrong_type!=PORT_V37_SCHED_CORRUPT||
       memcmp(candidate_links,f.acb[4],sizeof candidate_links)||
       f.arrivals!=2u||f.current_levels!=2u)
        return fail("non-ACB/IRP candidate rejected atomically");

    put32(f.acb[2]+4u,0xdeadbeefu);
    make_acb(&f,4u,2u,seed.pid);
    memcpy(candidate_links,f.acb[4],sizeof candidate_links);
    const uint32_t malformed=port_v37_sched_queue_ast(&f.scheduler,
        0x1000u,0x3400u);
    if(malformed!=PORT_V37_SCHED_CORRUPT||
       memcmp(candidate_links,f.acb[4],sizeof candidate_links)||
       f.arrivals!=2u||f.current_levels!=2u)
        return fail("corrupt existing AST queue rejected atomically");

    put32(f.acb[2]+4u,0x1010u);
    const uint32_t wq_va=address(&f,
        &f.scheduler.wait_headers[PORT_V37_SCH_MWAIT]);
    struct port_v37_wait_queue *wait=
        &f.scheduler.wait_headers[PORT_V37_SCH_MWAIT];
    wait->forward=wait->backward=0x1000u;
    wait->count=1u;
    f.scheduler.current_pcb=0u;
    put32(f.pcb+PORT_V37_PCB_SQFL,wq_va);
    put32(f.pcb+PORT_V37_PCB_SQBL,wq_va);
    put16(f.pcb+PORT_V37_PCB_STATE,PORT_V37_SCH_MWAIT);
    make_acb(&f,4u,1u,seed.pid);
    const uint32_t wake_status=port_v37_sched_queue_ast_rse(&f.scheduler,
        0x1000u,0x3400u,0u,0u);
    if(wake_status!=PORT_V37_SCHED_OK||
       wait->forward!=wq_va||wait->backward!=wq_va||wait->count!=0u||
       (uint16_t)(f.pcb[PORT_V37_PCB_STATE]|
          ((uint16_t)f.pcb[PORT_V37_PCB_STATE+1u]<<8))!=
              PORT_V37_SCH_COM||f.arrivals!=3u||
       port_v37_sched_check(&f.scheduler)!=PORT_V37_SCHED_OK){
        fprintf(stderr,"wake status=%u wqh=%08x/%08x count=%u state=%u "
            "pcbstate=%u arrivals=%u sched=%u\n",wake_status,
            wait->forward,wait->backward,wait->count,wait->state,
            (unsigned)(f.pcb[PORT_V37_PCB_STATE]|(
                (uint16_t)f.pcb[PORT_V37_PCB_STATE+1u]<<8)),f.arrivals,
            port_v37_sched_check(&f.scheduler));
        return fail("SCH$QAST arrival wakes resident AST waiter through RSE");
    }
    static const unsigned indexed_ast_wait_states[]={
        PORT_V37_SCH_CEF,PORT_V37_SCH_COLPG,PORT_V37_SCH_MWAIT,PORT_V37_SCH_PFW,
        PORT_V37_SCH_LEF,PORT_V37_SCH_LEFO,PORT_V37_SCH_HIB,
        PORT_V37_SCH_HIBO,PORT_V37_SCH_FPG};
    for(unsigned i=0;i<sizeof indexed_ast_wait_states/
            sizeof indexed_ast_wait_states[0];i++)
        if(!auto_wait_state(indexed_ast_wait_states[i]))
            return fail("automatic SCH$AQ_WQHDR AST wake for source state");
    puts("ASTDEL SCH$QAST source PCB queue ordering and PID checks PASS");
    return 0;
}
