#include "vms_v37_sched.h"
#include "vms_v37_ia32_context.h"
#include "vms_v37_phd.h"
#include <stdio.h>
#include <string.h>

struct fixture {
    struct port_v37_scheduler scheduler;
    uint32_t synchronization_enters,synchronization_leaves;
    uint8_t pcb[PORT_V37_PCB_BYTES];
    uint8_t phd[PORT_V37_PHD_BYTES];
    struct port_v37_ia32_restore_frame frame;
    struct port_v37_wait_queue lef;
    uint8_t ceb[PORT_V37_CEB_BYTES];
};
enum {PCB_VA=0x00200000u,PHD_VA=0x00500000u,FRAME_VA=0x00400000u,
      CEB_VA=0x00700000u,WQ_VA=0x00600000u,HEAD_VA=0x00100000u};
static uint32_t address(void *ctx,const void *p){
    struct fixture *f=ctx;const uintptr_t a=(uintptr_t)p;
    const uintptr_t b=(uintptr_t)&f->scheduler;
    if(a>=b&&a<=b+sizeof f->scheduler)
        return HEAD_VA+(uint32_t)(a-b);
    return 0u;
}
static uint8_t *resolve(void *ctx,uint32_t va,size_t n){
    struct fixture *f=ctx;
    const uint32_t h_end=HEAD_VA+sizeof f->scheduler;
    if(va>=HEAD_VA&&va<=h_end&&n<=h_end-va)
        return (uint8_t *)&f->scheduler+va-HEAD_VA;
    if(va>=PCB_VA&&va<=PCB_VA+sizeof f->pcb&&n<=PCB_VA+sizeof f->pcb-va)
        return f->pcb+va-PCB_VA;
    if(va>=PHD_VA&&va<=PHD_VA+sizeof f->phd&&n<=PHD_VA+sizeof f->phd-va)
        return f->phd+va-PHD_VA;
    if(va>=FRAME_VA&&va<=FRAME_VA+sizeof f->frame&&n<=FRAME_VA+sizeof f->frame-va)
        return (uint8_t *)&f->frame+va-FRAME_VA;
    if(va>=CEB_VA&&va<=CEB_VA+sizeof f->ceb&&n<=CEB_VA+sizeof f->ceb-va)
        return f->ceb+va-CEB_VA;
    if(va>=WQ_VA&&va<=WQ_VA+sizeof f->lef&&n<=WQ_VA+sizeof f->lef-va)
        return (uint8_t *)&f->lef+va-WQ_VA;
    return 0;
}
static void put16(uint8_t *p,uint16_t v){p[0]=(uint8_t)v;p[1]=(uint8_t)(v>>8);}
static uint16_t get16(const uint8_t *p){return (uint16_t)p[0]|(uint16_t)(p[1]<<8);}
static void put32(uint8_t *p,uint32_t v){
    p[0]=(uint8_t)v;p[1]=(uint8_t)(v>>8);p[2]=(uint8_t)(v>>16);p[3]=(uint8_t)(v>>24);
}
static uint32_t get32(const uint8_t *p){
    return (uint32_t)p[0]|((uint32_t)p[1]<<8)|((uint32_t)p[2]<<16)|((uint32_t)p[3]<<24);
}
static uint32_t enter_synchronization(void *ctx){
    struct fixture *f=ctx;f->synchronization_enters++;return 0u;
}
static void leave_synchronization(void *ctx,uint32_t token){
    struct fixture *f=ctx;if(token==0u)f->synchronization_leaves++;
}
static int fail(const char *what){fprintf(stderr,"VMS EVENT WAIT FAIL: %s\n",what);return 1;}
static int start_current(struct fixture *f,uint32_t *pcb,uint32_t *frame){
    uint32_t status=port_v37_sched_select(&f->scheduler,pcb,frame);
    if(status!=PORT_V37_SCHED_OK||*pcb!=PCB_VA||*frame!=FRAME_VA)
        fprintf(stderr,"select status=%u pcb=%08x frame=%08x check=%u\n",
            status,*pcb,*frame,port_v37_sched_check(&f->scheduler));
    return status==PORT_V37_SCHED_OK&&*pcb==PCB_VA&&*frame==FRAME_VA;
}
int main(void){
    struct fixture f={0};
    f.lef.forward=f.lef.backward=WQ_VA;f.lef.state=PORT_V37_SCH_LEF;
    const struct port_v37_sched_ops ops={.context=&f,.address=address,
        .resolve=resolve,.enter_synchronization=enter_synchronization,
        .leave_synchronization=leave_synchronization};
    if(port_v37_sched_init(&f.scheduler,&ops)!=PORT_V37_SCHED_OK)
        return fail("scheduler initialization");
    const struct port_v37_pcb_seed seed={.address=PCB_VA,.pid=0x00010000u,
        .phd_address=PHD_VA,.priority=15u,.name="WAIT",.name_bytes=4u};
    if(port_v37_pcb_init_genpcb(f.pcb,sizeof f.pcb,&seed)!=PORT_V37_PCB_OK)
        return fail("source-layout PCB initialization");
    f.frame=(struct port_v37_ia32_restore_frame){.gs=0x10u,.fs=0x10u,
        .es=0x10u,.ds=0x10u,.registers={.eip=0x12345678u,.cs=0x08u,.eflags=2u}};
    put32(f.phd+PORT_V37_PHD_KSP,FRAME_VA);
    put32(f.phd+PORT_V37_PHD_PC,f.frame.registers.eip);
    if(port_v37_pcb_bind_context(f.pcb,sizeof f.pcb,PHD_VA,FRAME_VA)!=PORT_V37_PCB_OK||
       port_v37_sched_chse(&f.scheduler,PCB_VA,0u)!=PORT_V37_SCHED_OK)
        return fail("resident PCB context and COM queue");
    uint32_t pcb=0u,frame=0u,ss=0u;
    if(!start_current(&f,&pcb,&frame))return fail("initial SCH$SCHED selection");

    if(port_v37_sched_wait_event(&f.scheduler,WQ_VA,FRAME_VA,1u,3u,0,
           &pcb,&frame,&ss)!=PORT_V37_SCHED_BLOCKED||ss!=1u||
       f.lef.count!=1u||get16(f.pcb+PORT_V37_PCB_STATE)!=PORT_V37_SCH_LEF||
       get32(f.pcb+PORT_V37_PCB_EFWM)!=~3u||
       (get32(f.pcb+PORT_V37_PCB_STS)&PORT_V37_PCB_STS_WALL))
        return fail("WFLOR stores local mask and blocks on LEF WQH");
    if(port_v37_sched_post_event(&f.scheduler,PCB_VA,WQ_VA,1u,0u,&ss)!=
           PORT_V37_SCHED_OK||ss!=1u||f.lef.count||
       get16(f.pcb+PORT_V37_PCB_STATE)!=PORT_V37_SCH_COM||
       f.frame.registers.eax!=1u)
        return fail("SETEF satisfies WFLOR and completes native frame");
    if(!start_current(&f,&pcb,&frame))return fail("reschedule after WFLOR");

    if(port_v37_sched_wait_event(&f.scheduler,WQ_VA,FRAME_VA,0u,3u,1,
           &pcb,&frame,&ss)!=PORT_V37_SCHED_BLOCKED||ss!=1u||
       f.lef.count!=1u||get32(f.pcb+PORT_V37_PCB_EFWM)!=~1u||
       !(get32(f.pcb+PORT_V37_PCB_STS)&PORT_V37_PCB_STS_WALL))
        return fail("WFLAND waits only for missing local flags");
    if(port_v37_sched_post_event(&f.scheduler,PCB_VA,WQ_VA,0u,0u,&ss)!=
           PORT_V37_SCHED_OK||ss!=1u||f.lef.count||
       get16(f.pcb+PORT_V37_PCB_STATE)!=PORT_V37_SCH_COM||
       f.frame.registers.eax!=1u)
        return fail("SETEF satisfies WFLAND after its final flag");

    put16(f.ceb+8u,PORT_V37_CEB_BYTES);
    f.ceb[PORT_V37_CEB_TYPE]=PORT_V37_CEB_NORMAL;
    put32(f.ceb+PORT_V37_CEB_WQH,CEB_VA+PORT_V37_CEB_WQH);
    put32(f.ceb+PORT_V37_CEB_WQH+4u,CEB_VA+PORT_V37_CEB_WQH);
    put16(f.ceb+PORT_V37_CEB_WQH+10u,PORT_V37_SCH_CEF);
    put32(f.pcb+PORT_V37_PCB_EFC2P,CEB_VA);
    if(!start_current(&f,&pcb,&frame))return fail("reschedule before common wait");
    if(port_v37_sched_wait_event(&f.scheduler,0u,FRAME_VA,64u,3u,1,
           &pcb,&frame,&ss)!=PORT_V37_SCHED_BLOCKED||ss!=1u||
       get16(f.ceb+PORT_V37_CEB_WQH+8u)!=1u)
        return fail("WFLAND selects CEB event vector and embedded WQH");
    if(port_v37_sched_post_event(&f.scheduler,PCB_VA,0u,64u,0u,&ss)!=
           PORT_V37_SCHED_OK||ss!=1u||get16(f.ceb+PORT_V37_CEB_WQH+8u)!=1u)
        return fail("common event leaves AND waiter blocked after first flag");
    const uint32_t saved_pc=get32(f.phd+PORT_V37_PHD_PC);
    const uint32_t saved_r0=get32(f.phd+PORT_V37_PHD_R0);
    const uint32_t saved_eax=f.frame.registers.eax;
    put32(f.pcb+PORT_V37_PCB_STS,
        get32(f.pcb+PORT_V37_PCB_STS)&~PORT_V37_PCB_STS_RESIDENT);
    const uint32_t common_post=port_v37_sched_post_event(&f.scheduler,
        PCB_VA,0u,65u,0u,&ss);
    if(common_post!=PORT_V37_SCHED_OK||ss!=1u||
       get16(f.ceb+PORT_V37_CEB_WQH+8u)||
       get16(f.pcb+PORT_V37_PCB_STATE)!=PORT_V37_SCH_COMO||
       get32(f.phd+PORT_V37_PHD_PC)!=saved_pc||
       get32(f.phd+PORT_V37_PHD_R0)!=saved_r0||
       f.frame.registers.eax!=saved_eax){
        fprintf(stderr,"post=%u ss=%u count=%u state=%u eax=%x flags=%08x efwm=%08x wall=%x\n",
            common_post,ss,get16(f.ceb+PORT_V37_CEB_WQH+8u),
            get16(f.pcb+PORT_V37_PCB_STATE),f.frame.registers.eax,
            get32(f.ceb+PORT_V37_CEB_EFC),get32(f.pcb+PORT_V37_PCB_EFWM),
            get32(f.pcb+PORT_V37_PCB_STS));
        return fail("CEF post wakes nonresident WFLAND without touching saved context");
    }
    put32(f.pcb+PORT_V37_PCB_STS,
        get32(f.pcb+PORT_V37_PCB_STS)|PORT_V37_PCB_STS_RESIDENT);
    if(port_v37_sched_chse(&f.scheduler,PCB_VA,0u)!=PORT_V37_SCHED_OK||
       get16(f.pcb+PORT_V37_PCB_STATE)!=PORT_V37_SCH_COM||
       !start_current(&f,&pcb,&frame))return fail("final ready queue selection");
    put16(f.ceb+8u,PORT_V37_CEB_MIN_BYTES-1u);
    ss=0xdeadbeefu;
    if(port_v37_sched_wait_event(&f.scheduler,0u,FRAME_VA,64u,1u,0,
           &pcb,&frame,&ss)!=PORT_V37_SCHED_CORRUPT||ss!=0xdeadbeefu||
       get32(f.ceb+PORT_V37_CEB_EFC)!=3u)
        return fail("truncated CEB rejected before service or flag mutation");
    put16(f.ceb+8u,PORT_V37_CEB_BYTES);
    f.ceb[PORT_V37_CEB_TYPE]=PORT_V37_CEB_SLAVE;
    ss=0xdeadbeefu;
    if(port_v37_sched_wait_event(&f.scheduler,0u,FRAME_VA,64u,1u,0,
           &pcb,&frame,&ss)!=PORT_V37_SCHED_UNRESOLVED||ss!=0xdeadbeefu||
       port_v37_sched_post_event(&f.scheduler,PCB_VA,0u,64u,0u,&ss)!=
           PORT_V37_SCHED_UNRESOLVED||ss!=0xdeadbeefu||
       get32(f.ceb+PORT_V37_CEB_EFC)!=3u)
        return fail("assigned slave CEB is unresolved, never misreported UNASEFC");
    f.ceb[PORT_V37_CEB_TYPE]=PORT_V37_CEB_NORMAL;
    if(port_v37_sched_wait_event(&f.scheduler,0u,FRAME_VA,128u,1u,0,
           &pcb,&frame,&ss)!=PORT_V37_SCHED_OK||ss!=PORT_V37_SS_ILLEFC||
       port_v37_sched_wait_event(&f.scheduler,0u,FRAME_VA,96u,1u,0,
           &pcb,&frame,&ss)!=PORT_V37_SCHED_OK||ss!=PORT_V37_SS_UNASEFC)
        return fail("illegal and unassigned event clusters return source statuses");
    if(!f.synchronization_enters||
       f.synchronization_enters!=f.synchronization_leaves)
        return fail("event flag calls restore the caller synchronization level");
    puts("SYSWAIT WFLOR/WFLAND local+common flags, POSTEF wake, IPL restore and IA-32 completion PASS");
    return 0;
}
