#include "vms_exe_canevt.h"
#include <stdio.h>
#include <assert.h>
#include <string.h>
struct fixture {
    struct port_exe_timer_queue q;
    struct port_exe_timer_process p;
    struct port_exe_timer_job j;
    struct port_exe_event_process ev;
    struct port_exe_timer_ops ops;
    uint64_t times[3];
};
static int read_time(void *ctx,uint32_t adr,void *buf,uint32_t bytes) {
    struct fixture *f=ctx;
    if(adr<1 || adr>3 || bytes!=8)return 0;
    memcpy(buf,&f->times[adr-1],8);return 1;
}
static uint32_t resolve(void *ctx,uint32_t adr,uint32_t name,
                        uint32_t owner,uint32_t *out) {
    (void)ctx;(void)name;
    if(adr==25u && owner==7u){*out=8u;return 1u;}
    return PORT_SS_NONEXPR;
}
static uint32_t wake(void *ctx,uint32_t pid){(void)ctx;(void)pid;return 1u;}
static uint32_t post(void *ctx,uint32_t pid,uint32_t efn,uint32_t priority){
    (void)ctx;(void)pid;(void)efn;(void)priority;return 1u;
}
static void setup(struct fixture *f) {
    memset(f,0,sizeof(*f));
    port_exe_timer_init(&f->q,UINT64_C(1000000));
    f->times[0]=(uint64_t)(int64_t)-200000;
    f->times[1]=(uint64_t)(int64_t)-300000;
    f->times[2]=(uint64_t)(int64_t)-100000;
    f->p.pid=7;f->p.previous_mode=3;
    f->p.ast_available=8;f->p.job=&f->j;
    f->j.timer_available=6;
    port_exe_event_init(&f->ev,7);f->p.events=&f->ev;
    f->ops.context=f;f->ops.read_user=read_time;
    f->ops.resolve_process=resolve;f->ops.wake_process=wake;
    f->ops.post_event=post;
}
int main(void) {
    struct fixture f;setup(&f);
    struct port_exe_timer_request t={5,1,0,17};
    assert(port_exe_setimr(&f.q,&f.p,&t,&f.ops)==1);
    f.p.previous_mode=1;
    t.request_id=18;t.day_time=2;
    assert(port_exe_setimr(&f.q,&f.p,&t,&f.ops)==1);
    f.p.previous_mode=3;
    t.request_id=19;t.day_time=3;
    assert(port_exe_setimr(&f.q,&f.p,&t,&f.ops)==1);
    assert(f.j.timer_available==3 && f.q.used==3);
    unsigned n=99;
    assert(port_exe_cantim(&f.q,&f.p,19,0,&n)==1 && n==1);
    assert(f.j.timer_available==4 && f.q.used==2);
    f.p.previous_mode=0;
    assert(port_exe_cantim(&f.q,&f.p,18,1,&n)==1 && n==1);
    assert(port_exe_cantim(&f.q,&f.p,99,0,&n)==1 && n==0);
    assert(port_exe_cantim(&f.q,&f.p,17,0,&n)==1 && n==1);
    assert(f.j.timer_available==6 && f.q.used==0);
    struct port_exe_wake_request w={25,0,1,0};
    assert(port_exe_schdwk(&f.q,&f.p,&w,&f.ops)==1);
    w.pid_address=0;w.repeat_time=2;
    assert(port_exe_schdwk(&f.q,&f.p,&w,&f.ops)==1);
    assert(f.q.used==2 && f.p.ast_available==6);
    struct port_exe_cancel_wake_request c={25,0};
    assert(port_exe_canwak(&f.q,&f.p,&c,&f.ops,&n)==1 && n==1);
    assert(f.q.used==1 && f.p.ast_available==7);
    c.pid_address=0;
    assert(port_exe_canwak(&f.q,&f.p,&c,&f.ops,&n)==1 && n==1);
    assert(f.q.used==0 && f.p.ast_available==8);
    assert(port_exe_canwak(&f.q,&f.p,&c,&f.ops,&n)==1 && n==0);
    c.pid_address=99;
    assert(port_exe_canwak(&f.q,&f.p,&c,&f.ops,&n)==PORT_SS_NONEXPR);
    puts("SYSCANEVT queue cancellation and quotas pass");
    return 0;
}
