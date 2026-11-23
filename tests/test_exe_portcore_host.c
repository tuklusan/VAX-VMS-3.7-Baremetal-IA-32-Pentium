#include "vms_exe_chgmod.h"
#include "vms_exe_dclcmh.h"
#include "vms_exe_sched.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>
struct test {
    unsigned calls,loaded,saved,idle,fatal,bugs;
    uint8_t target;
    uint32_t address,argv,words[64];
    struct port_exe_scheduler *scheduler;
    struct port_exe_sched_process *waiting;
};
static int read_mem(void *c,uint32_t a,void *d,uint32_t n) {
    struct test *t=c;
    if(a+n>sizeof(t->words))return 0;
    memcpy(d,(unsigned char *)t->words+a,n);return 1;
}
static int write_mem(void *c,uint32_t a,const void *d,uint32_t n) {
    struct test *t=c;
    if(a+n>sizeof(t->words))return 0;
    memcpy((unsigned char *)t->words+a,d,n);return 1;
}
static int probe(void *c,uint32_t a,uint32_t n) {
    struct test *t=c;
    return a+n<=sizeof(t->words) && a!=252u;
}
static uint32_t run_mode(void *c,uint8_t mode,uint32_t routine,uint32_t argv) {
    struct test *t=c;
    t->target=mode;t->address=routine;t->argv=argv;t->calls++;
    return routine==4?PORT_SS_NORMAL:PORT_SS_ACCVIO;
}
static void badcall(void *c) {((struct test *)c)->bugs++;}
static void save(void *c,struct port_exe_sched_process *p) {
    struct test *t=c;
    t->saved++;
    assert(p && p->state==PORT_EXE_STATE_CURRENT);
}
static void load(void *c,struct port_exe_sched_process *p) {
    struct test *t=c;
    t->loaded++;
    assert(p && p->state==PORT_EXE_STATE_CURRENT);
}
static void idle(void *c) {
    struct test *t=c;
    t->idle++;
    assert(t->waiting && t->idle==1u);
    assert(port_exe_sched_enqueue(t->scheduler,t->waiting)==PORT_SS_NORMAL);
}
static void fatal(void *c) {((struct test *)c)->fatal++;}
int main(void) {
    struct test t={0};
    struct port_exe_mode_executor exec={&t,run_mode};
    struct port_exe_mode_context c={.previous_mode=3};
    assert(port_exe_cmexec(&c,4,55,&exec)==PORT_SS_NOPRIV);
    assert(port_exe_cmkrnl(&c,4,55,&exec)==PORT_SS_NOPRIV);
    c.has_cmexec=1;
    assert(port_exe_cmexec(&c,4,55,&exec)==1u);
    assert(t.target==1u && t.address==4 && t.argv==55 && t.calls==1);
    assert(port_exe_cmkrnl(&c,4,55,&exec)==PORT_SS_NOPRIV);
    c.has_cmexec=0;c.has_cmkrnl=1;
    assert(port_exe_cmexec(&c,4,55,&exec)==1u && t.target==1u);
    assert(port_exe_cmkrnl(&c,4,55,&exec)==1u && t.target==0u);
    assert(port_exe_cmkrnl(&c,8,55,&exec)==PORT_SS_ACCVIO);
    c.has_cmkrnl=0;c.previous_mode=0;
    assert(port_exe_cmkrnl(&c,4,55,&exec)==1u);

    struct port_exe_access user={&t,read_mem,write_mem,probe,probe};
    struct port_exe_bad_service bad={&t,badcall};
    struct port_exe_change_handlers h={.previous_mode=3,
        .supervisor=10,.user=20,.compatibility=30};
    assert(port_exe_dclcmh(&h,123,0,0,&user,&bad)==1u);
    assert(h.user==123u);
    assert(port_exe_dclcmh(&h,44,4,1,&user,&bad)==1u);
    assert(h.compatibility==44u && t.words[1]==30u);
    assert(port_exe_dclcmh(&h,66,252,1,&user,&bad)==PORT_SS_ACCVIO);
    assert(h.compatibility==44u);
    h.previous_mode=2;
    assert(port_exe_dclcmh(&h,77,0,0,&user,&bad)==1u && h.supervisor==77);
    h.previous_mode=1;
    assert(port_exe_dclcmh(&h,77,0,0,&user,&bad)==PORT_SS_IVSSRQ);
    assert(t.bugs==1u);
    assert(port_exe_dclcmh(&h,88,0,1,&user,&bad)==1u && h.compatibility==88u);

    struct port_exe_scheduler sched;
    port_exe_sched_init(&sched,0x31u);
    struct port_exe_sched_process a={.pid=11,.type=0x31u,
        .current_priority=20,.base_priority=22,.state=PORT_EXE_STATE_OTHER};
    struct port_exe_sched_process b={.pid=12,.type=0x31u,
        .current_priority=4,.base_priority=4,.state=PORT_EXE_STATE_OTHER};
    struct port_exe_sched_process d={.pid=13,.type=0x31u,
        .current_priority=20,.base_priority=22,.state=PORT_EXE_STATE_OTHER};
    struct port_exe_sched_ops ops={&t,save,load,idle,fatal};
    assert(port_exe_sched_enqueue(&sched,&a)==1u);
    assert(port_exe_sched_enqueue(&sched,&b)==1u);
    assert(port_exe_sched_enqueue(&sched,&d)==1u);
    assert(port_exe_sched_select(&sched,&ops)==1u);
    assert(sched.current==&b && sched.current_priority==4);
    assert(port_exe_sched_resched(&sched,&ops)==1u);
    assert(sched.current==&b && t.saved==1 && t.loaded==2);
    assert(sched.ready_mask&(1u<<20));
    assert(port_exe_sched_enqueue(&sched,&a)==PORT_SS_BADPARAM);
    assert(port_exe_sched_resched(&sched,&ops)==1u);
    assert(sched.current==&b);
    /* With no real-time process, FIFO rotates within the same queue. */
    struct port_exe_scheduler idle_sched;
    port_exe_sched_init(&idle_sched,0x31u);
    t.scheduler=&idle_sched;t.waiting=&a;
    a.state=PORT_EXE_STATE_OTHER;a.next=a.previous=NULL;
    assert(port_exe_sched_select(&idle_sched,&ops)==1u);
    assert(t.idle==1 && t.loaded==4 && idle_sched.current==&a);
    assert(idle_sched.current_priority==21u && a.current_priority==21u);
    struct port_exe_scheduler broken;
    port_exe_sched_init(&broken,0x31u);
    broken.ready_mask=1u<<3;
    assert(port_exe_sched_select(&broken,&ops)==PORT_SS_BADQUEUEHDR);
    assert(t.fatal==1u);
    /* A blocked/deleted current PCB is saved but never reinserted. An
     * already-ready process receives the next context. */
    struct port_exe_scheduler suspended;
    struct port_exe_sched_process running={.pid=21,.type=0x31u,
        .current_priority=9,.base_priority=9,.state=PORT_EXE_STATE_CURRENT};
    struct port_exe_sched_process ready={.pid=22,.type=0x31u,
        .current_priority=3,.base_priority=3,.state=PORT_EXE_STATE_OTHER};
    port_exe_sched_init(&suspended,0x31u);
    suspended.current=&running;
    assert(port_exe_sched_enqueue(&suspended,&ready)==PORT_SS_NORMAL);
    unsigned saved_before=t.saved,loaded_before=t.loaded;
    assert(port_exe_sched_suspend(&suspended,&ops)==PORT_SS_NORMAL);
    assert(t.saved==saved_before+1u && t.loaded==loaded_before+1u);
    assert(suspended.current==&ready && ready.state==PORT_EXE_STATE_CURRENT);
    assert(running.state==PORT_EXE_STATE_OTHER &&
           !(suspended.ready_mask&(1u<<running.current_priority)));
    suspended.current=NULL;
    assert(port_exe_sched_suspend(&suspended,&ops)==PORT_SS_BADPARAM);
    puts("SYSCHGMOD SYSDCLCMH SCHED pass");
    return 0;
}
