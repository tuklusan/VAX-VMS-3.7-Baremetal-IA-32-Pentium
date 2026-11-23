#include "vms_exe_setstk.h"
#include "vms_exe_setprv.h"
#include "vms_exe_setpri.h"
#include "vms_exe_setpra.h"
#include "vms_exe_adjstk.h"
#include "vms_exe_derlmb.h"
#include "vms_exe_getpti.h"
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
struct test {
    uint8_t bytes[8192];
    uint32_t blocked,block_end;
    unsigned locks,unlocks,events,expanded,special,normal;
    uint32_t ast_pid,ast_parameter,ast_address,priority;
    uint8_t ast_mode;
    struct port_exe_priority_process *target;
    uint8_t pages[16],frames[16],global_type[16],dzro[16];
};
static int range(struct test *t,uint32_t a,uint32_t n) {
    return a<sizeof(t->bytes) && n<=sizeof(t->bytes)-a &&
           !(t->block_end>t->blocked && a<t->block_end &&
             (uint64_t)a+n>t->blocked);
}
static int rd(void *c,uint32_t a,void *out,uint32_t n) {
    struct test *t=c;
    if(!range(t,a,n))return 0;
    memcpy(out,t->bytes+a,n);
    return 1;
}
static int wr(void *c,uint32_t a,const void *v,uint32_t n) {
    struct test *t=c;
    if(!range(t,a,n))return 0;
    memcpy(t->bytes+a,v,n);
    return 1;
}
static int can(void *c,uint32_t a,uint32_t n) {return range(c,a,n);}
static void poke(struct test *t,uint32_t a,const void *p,unsigned n) {
    assert(a+n<=sizeof(t->bytes));
    memcpy(t->bytes+a,p,n);
}
static uint32_t get32(struct test *t,unsigned a) {
    uint32_t v;
    memcpy(&v,t->bytes+a,4);
    return v;
}
static uint64_t get64(struct test *t,unsigned a) {
    uint64_t v;
    memcpy(&v,t->bytes+a,8);
    return v;
}
static void lock(void *c){((struct test *)c)->locks++;}
static void unlock(void *c){((struct test *)c)->unlocks++;}
static void priority(void *c,struct port_exe_priority_process *target,uint8_t n) {
    struct test *t=c;
    assert(t->target==target);
    t->events++;
    t->priority=n;
}
static uint32_t resolve(void *c,uint32_t pid_ptr,uint32_t name_ptr,uint32_t caller,
                        struct port_exe_priority_process **out) {
    struct test *t=c;
    assert(caller==3u && pid_ptr==8u && name_ptr==0u);
    *out=t->target;
    return 1;
}
static uint32_t queue_special(void *c,uint32_t pid,uint32_t length) {
    struct test *t=c;
    t->special++;t->ast_pid=pid;t->ast_parameter=length;
    return PORT_SS_NORMAL;
}
static uint32_t queue_normal(void *c,uint32_t pid,uint32_t addr,uint32_t length,uint8_t mode) {
    struct test *t=c;
    t->normal++;t->ast_pid=pid;t->ast_address=addr;
    t->ast_parameter=length;t->ast_mode=mode;
    return PORT_SS_NORMAL;
}
static int stack_probe(void *c,uint8_t mode,uint32_t a,uint32_t n) {
    struct test *t=c;
    assert(mode==3u && n<=512u);
    return a>=t->blocked;
}
static uint32_t stack_expand(void *c,uint32_t a) {
    struct test *t=c;
    t->expanded++;
    t->blocked=a;
    return PORT_SS_NORMAL;
}
static uint32_t inspect(void *c,uint32_t va,uint8_t mode,
                        struct port_exe_pti_page *out) {
    struct test *t=c;
    if(mode!=3u)return PORT_SS_PAGOWNVIO;
    unsigned i=va/512u;
    if(i>=16)return PORT_SS_LENVIO;
    if(t->pages[i]==255u)return PORT_SS_PAGOWNVIO;
    out->type=t->pages[i];
    out->frame_or_section=t->frames[i];
    out->global_type=t->global_type[i];
    out->global_valid=0;
    out->section_demand_zero=t->dzro[i];
    return PORT_SS_NORMAL;
}
int main(void) {
    struct test t={0};
    const struct port_exe_access memory={&t,rd,wr,can,can};
    struct port_exe_stack_bounds st={.previous_mode=2};
    uint32_t new_stk[2]={0x4000u,0x8000u};
    poke(&t,128u,new_stk,8u);
    st.low[2]=0x1000;st.high[2]=0x2000;
    assert(port_exe_setstk(&st,128,256,0,&memory)==1u);
    assert(st.low[2]==0x4000 && st.high[2]==0x8000);
    assert(get32(&t,256)==0x1000 && get32(&t,260)==0x2000);
    t.blocked=260;t.block_end=264;
    assert(port_exe_setstk(&st,128,256,2,&memory)==PORT_SS_ACCVIO);
    assert(st.low[2]==0x4000);
    t.block_end=0;
    assert(port_exe_setstk(&st,0,0,3,&memory)==1u);
    assert(port_exe_setstk(&st,0,0,2,&memory)==PORT_SS_ACCVIO);

    struct port_exe_privileges prv={.current=4u,.process_header=4u,
                                   .permanent=8u,.authorized=0x34u,
                                   .image=UINT64_C(1)<<40,.previous_mode=3};
    uint64_t requested=(UINT64_C(1)<<63)|(UINT64_C(1)<<40)|0x10u;
    poke(&t,512u,&requested,8u);
    struct port_exe_setprv_request pr={1u,512u,0u,640u};
    assert(port_exe_setprv(&prv,&pr,&memory)==PORT_SS_NOTALLPRIV);
    assert(get64(&t,640u)==4u);
    assert(prv.current==(4u|0x10u|(UINT64_C(1)<<40)) &&
           prv.process_header==prv.current && prv.permanent==8u);
    pr.permanent_flag=1u;
    assert(port_exe_setprv(&prv,&pr,&memory)==PORT_SS_NOTALLPRIV);
    assert(prv.permanent==(8u|0x10u));
    assert(get64(&t,640u)==8u);
    pr.enable_flag=0u;
    assert(port_exe_setprv(&prv,&pr,&memory)==1u);
    assert(prv.current==4u && prv.permanent==8u);
    pr.permanent_flag=2u;
    assert(port_exe_setprv(&prv,&pr,&memory)==PORT_SS_IVSTSFLG);
    pr.permanent_flag=0;
    t.blocked=640;t.block_end=648;
    assert(port_exe_setprv(&prv,&pr,&memory)==PORT_SS_ACCVIO);
    assert(prv.current==4u);
    t.block_end=0;
    prv.authorized=(UINT64_C(1)<<PORT_PRV_SETPRV_BIT);
    prv.current=0;prv.process_header=0;
    pr.enable_flag=1u;
    assert(port_exe_setprv(&prv,&pr,&memory)==1u && prv.current==requested);
    prv.current=0;prv.process_header=0;prv.previous_mode=1;prv.authorized=0;
    assert(port_exe_setprv(&prv,&pr,&memory)==1u && prv.current==requested);
    pr.privilege_address=0;
    assert(port_exe_setprv(&prv,&pr,&memory)==1u && get64(&t,640u)==requested);

    struct port_exe_priority_process caller={.pid=3,.base=22,
        .authorized_base=20,.saved_base=22};
    struct port_exe_priority_process target={.pid=9,.saved_base=24,
        .base=24,.current=24,.executing=1};
    t.target=&target;
    struct port_exe_setpri_request pri_req={8u,0u,15u,768u};
    struct port_exe_priority_ops pri_ops={&memory,&t,resolve,priority};
    assert(port_exe_setpri(&caller,&pri_req,&pri_ops)==1u);
    assert(target.saved_base==20 && target.base==20 && target.current==20);
    assert(get32(&t,768)==7u && t.events==1u && t.priority==20u);
    caller.can_set_any_priority=1;
    target.mutex_count=1;target.executing=0;
    pri_req.priority=31u;
    assert(port_exe_setpri(&caller,&pri_req,&pri_ops)==1u);
    assert(target.saved_base==0 && target.base==20 && t.events==1u);
    target.mutex_count=0;
    assert(port_exe_setpri(&caller,&pri_req,&pri_ops)==1u);
    assert(target.base==0 && target.current==20 && t.events==2u);
    t.blocked=768;t.block_end=772;
    assert(port_exe_setpri(&caller,&pri_req,&pri_ops)==PORT_SS_ACCVIO);
    assert(t.events==2);
    t.block_end=0;

    struct port_exe_power_process pw={.pid=12,.ast_available=2,.previous_mode=2};
    assert(port_exe_setpra(&pw,0x12345678u,0u)==1u);
    assert(pw.ast_available==1u && pw.ast_mode==2 && pw.power_pending==1);
    assert(port_exe_setpra(&pw,0x8888u,3u)==1u);
    assert(pw.ast_available==1 && pw.ast_mode==3);
    struct port_exe_power_process *slots[3]={NULL,&pw,NULL};
    struct port_exe_power_ops pwops={&t,queue_special,queue_normal};
    uint32_t power_time=222u;
    assert(port_exe_powerast(slots,3,&power_time,&pwops)==1u);
    assert(t.special==1u && t.ast_pid==12u && t.ast_parameter==222u);
    assert(power_time==0 && pw.power_pending==0);
    assert(port_exe_powerast_requeue(&pw,222,&pwops)==1u);
    assert(t.normal==1u && t.ast_address==0x8888u && t.ast_mode==3u);
    assert(pw.ast_address==0u);
    pw.ast_available=0;
    assert(port_exe_setpra(&pw,100,3)==PORT_SS_EXQUOTA);

    struct port_exe_outer_stacks outer={.previous_mode=1};
    outer.mode_sp[3]=4000u;
    uint32_t zero=0u;
    poke(&t,960u,&zero,4u);
    struct port_exe_adjust_stack_ops adjops={&memory,&t,stack_probe,stack_expand};
    t.blocked=3500;
    assert(port_exe_adjstk(&outer,1u,(uint32_t)-128,960,&adjops)==PORT_SS_NOPRIV);
    assert(port_exe_adjstk(&outer,3u,(uint32_t)-128,960,&adjops)==1u);
    assert(outer.mode_sp[3]==3872u && get32(&t,960u)==3872u);
    poke(&t,960u,&zero,4u);
    assert(port_exe_adjstk(&outer,3u,(uint32_t)-700,960,&adjops)==1u);
    assert(t.expanded==1 && outer.mode_sp[3]==3172u);
    t.blocked=960;t.block_end=964;
    assert(port_exe_adjstk(&outer,3u,4u,960,&adjops)==PORT_SS_ACCVIO);
    t.block_end=0;

    struct port_exe_error_mailbox mb={0};
    struct port_exe_erlog_ops er={&t,lock,unlock};
    assert(port_exe_derlmb(&mb,3,12,0,&er)==PORT_SS_NOPRIV);
    assert(t.locks==0);
    assert(port_exe_derlmb(&mb,3,12,1,&er)==1u);
    assert(mb.unit==12 && mb.owner_pid==3);
    assert(port_exe_derlmb(&mb,9,44,1,&er)==PORT_SS_DEVALLOC);
    assert(mb.unit==12 && mb.owner_pid==3);
    assert(port_exe_derlmb(&mb,3,0,1,&er)==1u);
    assert(mb.unit==0 && mb.owner_pid==0);
    assert(port_exe_derlmb(&mb,9,65539,1,&er)==1u);
    assert(mb.unit==3 && mb.owner_pid==9);
    assert(t.locks==t.unlocks && t.locks==4);

    struct port_exe_pti_ops ptops={&memory,&t,inspect};
    for(unsigned i=0;i<16;i++)t.pages[i]=PORT_PTI_VALID;
    t.pages[1]=t.pages[2]=PORT_PTI_TRANSITION;
    t.pages[4]=t.pages[5]=t.pages[6]=PORT_PTI_SECTION;
    t.dzro[4]=t.dzro[5]=t.dzro[6]=1;
    uint32_t vms_range[2]={0u,4095u};
    poke(&t,1024u,vms_range,8u);
    struct port_exe_pti_request pt={1024,1100,3,PORT_PTI_DEMAND_ZERO,3};
    assert(port_exe_getpti(3,&pt,&ptops)==1u);
    assert(get32(&t,1100u)==2048u && get32(&t,1104u)==3583u);
    pt.minimum_pages=4u;
    uint32_t old_return[2]={0xcccccccc,0xaaaaaaaa};
    poke(&t,1100u,old_return,8u);
    assert(port_exe_getpti(3,&pt,&ptops)==1u);
    assert(get32(&t,1100u)==old_return[0] && get32(&t,1104u)==old_return[1]);
    vms_range[0]=4095;vms_range[1]=0;
    poke(&t,1024u,vms_range,8u);
    pt.minimum_pages=3u;
    assert(port_exe_getpti(3,&pt,&ptops)==1u);
    assert(get32(&t,1100u)==3583u && get32(&t,1104u)==2048u);
    t.pages[7]=255u;
    vms_range[0]=3584;vms_range[1]=0;
    poke(&t,1024u,vms_range,8u);
    assert(port_exe_getpti(3,&pt,&ptops)==PORT_SS_PAGOWNVIO);
    t.pages[7]=PORT_PTI_VALID;
    pt.access_mode=0;
    assert(port_exe_getpti(0,&pt,&ptops)==PORT_SS_PAGOWNVIO);
    pt.access_mode=3;
    vms_range[0]=0x80000000u;vms_range[1]=0x800007ffu;
    poke(&t,1024u,vms_range,8u);
    assert(port_exe_getpti(3,&pt,&ptops)==PORT_SS_NOPRIV);
    puts("SYSSETSTK SYSSETPRV SYSSETPRI SYSSETPRA SYSADJSTK SYSDERLMB SYSGETPTI pass");
    return 0;
}
