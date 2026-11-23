#include "vms_exe_schevt.h"
#include "vms_postef.h"
#include <assert.h>
#include <stdint.h>
#include <string.h>
#include <stdio.h>

struct fixture {
    struct port_exe_timer_queue queue;
    struct port_exe_timer_process current;
    struct port_exe_timer_job job;
    struct port_exe_event_process events;
    struct port_exe_timer_ops ops;
    uint64_t times[4];
    struct port_postef_service scheduler;
    struct port_pcb pcb;
    unsigned wakes,asts,waits;
    uint32_t sequence[8];
    uint32_t ast_parameter,ast_target,ast_mode;
};
static int read_time(void *ctx,uint32_t addr,void *buffer,uint32_t bytes) {
    struct fixture *f=ctx;
    if(bytes!=8u || addr==0 || addr>4u)return 0;
    memcpy(buffer,&f->times[addr-1u],8);
    return 1;
}
static uint32_t resolve(void *ctx,uint32_t pidaddr,uint32_t name,
                        uint32_t requester,uint32_t *target) {
    (void)ctx; (void)name;
    if(pidaddr>=40 && pidaddr<=42 && requester==7) {
        *target=7+(pidaddr-40);return PORT_SS_NORMAL;
    }
    return PORT_SS_NONEXPR;
}
static uint32_t wake(void *ctx,uint32_t pid) {
    struct fixture *f=ctx;
    if(pid<7 || pid>9)return PORT_SS_NONEXPR;
    f->sequence[f->wakes]=pid;
    ++f->wakes;
    return PORT_SS_NORMAL;
}
static uint32_t post(void *ctx,uint32_t pid,uint32_t efn,uint32_t pri) {
    struct fixture *f=ctx;
    unsigned woken=0;
    int status=port_postef_post(&f->scheduler,pid,efn,pri,&woken);
    if(status<0)return PORT_SS_BADQUEUEHDR;
    /* Both views are backed by one logical process event cluster. */
    uint32_t mirrored=port_exe_event_post(f->current.events,efn);
    assert(mirrored&1u);
    return status==PORT_POSTEF_WAS_CLEAR?PORT_SS_WASCLR:PORT_SS_WASSET;
}
static uint32_t wait_resource(void *ctx,uint32_t pid,uint32_t why) {
    struct fixture *f=ctx;
    if(pid!=7)return PORT_SS_NONEXPR;
    if(why==PORT_EXE_WAIT_TIMER_QUOTA)++f->job.timer_available;
    else return PORT_SS_INSFMEM;
    ++f->waits;
    return PORT_SS_NORMAL;
}
static uint32_t ast(void *ctx,uint32_t pid,uint32_t handler,
                    uint32_t parameter,uint8_t mode) {
    struct fixture *f=ctx;
    if(pid!=7 || handler!=123u)return PORT_SS_BADQUEUEHDR;
    ++f->asts;f->ast_parameter=parameter;f->ast_target=pid;
    f->ast_mode=mode;
    return PORT_SS_NORMAL;
}
static void initialize(struct fixture *f) {
    memset(f,0,sizeof(*f));
    port_exe_timer_init(&f->queue,UINT64_C(500000000));
    port_exe_event_init(&f->events,7);
    f->current.pid=7;
    f->current.events=&f->events;
    f->current.ast_available=5;
    f->current.job=&f->job;
    f->job.timer_available=3;
    f->current.previous_mode=3;
    f->ops.context=f;
    f->ops.read_user=read_time;
    f->ops.resolve_process=resolve;
    f->ops.wake_process=wake;
    f->ops.post_event=post;
    f->ops.queue_ast=ast;
    f->ops.wait_resource=wait_resource;
    port_postef_init(&f->scheduler);
    assert(port_pcb_create(&f->pcb,7u,(uint32_t *)(uintptr_t)0x1000,
                           0x2000u,0x3000u)==0);
    assert(port_postef_register(&f->scheduler,&f->pcb)==0);
}
static void test_timer(void) {
    struct fixture f;initialize(&f);
    f.times[0]=(uint64_t)(int64_t)-200000; /* 20 ms */
    struct port_exe_timer_request r={31,1,123,456};
    assert(port_exe_event_post(&f.events,31)==PORT_SS_WASCLR);
    assert(port_exe_setimr(&f.queue,&f.current,&r,&f.ops)==PORT_SS_NORMAL);
    assert((f.events.local[0]&(1u<<31))==0);
    assert(f.job.timer_available==2 && f.current.ast_available==4);
    assert(port_exe_timer_advance(&f.queue,500100000u,&f.ops)==0);
    assert(port_exe_timer_advance(&f.queue,500200000u,&f.ops)==1);
    assert(f.scheduler.tasks[0].private_banks[0].flags&(1u<<31));
    assert(f.events.local[0]&(1u<<31));
    assert(f.asts==1 && f.ast_parameter==456 && f.ast_mode==3);
    assert(f.job.timer_available==3 && f.current.ast_available==5);
    assert(f.queue.used==0 && f.queue.failed==0);
}
static void test_wake_and_repeat(void) {
    struct fixture f;initialize(&f);
    f.times[0]=(uint64_t)(int64_t)-100000;
    f.times[1]=(uint64_t)(int64_t)-1; /* clamp to 10 ms */
    struct port_exe_wake_request w={40,0,1,2};
    assert(port_exe_schdwk(&f.queue,&f.current,&w,&f.ops)==PORT_SS_NORMAL);
    assert(f.current.ast_available==4 && f.queue.entries[0].interval==100000u);
    assert(port_exe_timer_advance(&f.queue,500100000u,&f.ops)==1);
    assert(f.wakes==1 && f.queue.used==1);
    assert(port_exe_timer_advance(&f.queue,500200000u,&f.ops)==1);
    assert(f.wakes==2 && f.current.ast_available==4);
    f.times[1]=500000; /* positive repeat prohibited */
    assert(port_exe_schdwk(&f.queue,&f.current,&w,&f.ops)==PORT_SS_IVTIME);
    w.pid_address=17;
    assert(port_exe_schdwk(&f.queue,&f.current,&w,&f.ops)==PORT_SS_NONEXPR);
}
static void test_expiry_order_and_capacity(void) {
    struct fixture f;initialize(&f);
    f.times[0]=(uint64_t)(int64_t)-300000;
    f.times[1]=(uint64_t)(int64_t)-100000;
    f.times[2]=(uint64_t)(int64_t)-200000;
    struct port_exe_wake_request req={0,0,1,0};
    for(unsigned i=0;i<3;i++) {
        req.day_time=i+1;req.pid_address=40+i;
        assert(port_exe_schdwk(&f.queue,&f.current,&req,&f.ops)==PORT_SS_NORMAL);
    }
    assert(port_exe_timer_advance(&f.queue,500300000u,&f.ops)==3);
    assert(f.wakes==3 && f.queue.used==0 && f.current.ast_available==5);
    assert(f.sequence[0]==8 && f.sequence[1]==9 && f.sequence[2]==7);
    /* Verify the queue chose ascending deadlines, not array slots. */
    f.current.ast_available=PORT_EXE_TQE_CAPACITY;
    f.times[0]=(uint64_t)(int64_t)-100000;
    req.day_time=1;
    for(unsigned i=0;i<PORT_EXE_TQE_CAPACITY;i++)
        assert(port_exe_schdwk(&f.queue,&f.current,&req,&f.ops)==PORT_SS_NORMAL);
    assert(port_exe_schdwk(&f.queue,&f.current,&req,&f.ops)==PORT_SS_EXQUOTA);
    f.current.ast_available=1;
    f.current.implicit_wait_disabled=1;
    assert(port_exe_schdwk(&f.queue,&f.current,&req,&f.ops)==PORT_SS_INSFMEM);
}
static void test_errors(void) {
    struct fixture f;initialize(&f);
    struct port_exe_timer_request t={31,1,123,8};
    f.times[0]=(uint64_t)(int64_t)-100000;
    f.current.ast_available=0;
    assert(port_exe_setimr(&f.queue,&f.current,&t,&f.ops)==PORT_SS_EXQUOTA);
    f.current.ast_available=1;
    f.job.timer_available=0;
    assert(port_exe_setimr(&f.queue,&f.current,&t,&f.ops)==PORT_SS_NORMAL);
    assert(f.waits==1 && f.job.timer_available==0);
    /* Remove this pending request before testing the next one. */
    assert(port_exe_timer_advance(&f.queue,500100000u,&f.ops)==1);
    f.job.timer_available=1;
    port_exe_timer_init(&f.queue,500000000u);
    t.efn=128;
    assert(port_exe_setimr(&f.queue,&f.current,&t,&f.ops)==PORT_SS_ILLEFC);
    t.efn=31;t.day_time=5;
    assert(port_exe_setimr(&f.queue,&f.current,&t,&f.ops)==PORT_SS_ACCVIO);
    t.day_time=1;
    assert(port_exe_setimr(&f.queue,&f.current,&t,&f.ops)==PORT_SS_NORMAL);
    assert(f.job.timer_available==0);
    /* No scheduler can complete an event timer without the posting path. */
    f.ops.post_event=0;
    assert(port_exe_timer_advance(&f.queue,500100000u,&f.ops)==1);
    assert(f.queue.failed==1 && f.queue.delivered==0);
}
int main(void) {
    test_timer();test_wake_and_repeat();test_expiry_order_and_capacity();test_errors();
    puts("SYSSCHEVT host tests passed");
    return 0;
}
