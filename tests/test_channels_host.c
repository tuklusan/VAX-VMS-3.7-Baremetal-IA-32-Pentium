#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include "vms_channels.h"
static int read_ok(void *ctx,uint32_t lba,uint16_t b[256]) {
    int *calls=(int *)ctx;
    ++*calls;
    for(unsigned i=0;i<256;i++)b[i]=(uint16_t)(lba+i);
    return 0;
}
static int read_other(void *ctx,uint32_t lba,uint16_t b[256]) {
    (void)ctx;(void)lba;(void)b;return -1;
}
static unsigned callback_count;
static void done(void *ctx,uint32_t p) { (void)ctx;assert(p==123);++callback_count; }
int main(void) {
    struct port_channel_table t;
    struct port_qio_service svc;
    struct port_event_cluster flags;
    struct vms_ast_process a,b;
    struct port_qio_request req[2]={{0}};
    struct port_qio_iosb iosb[2]={{0}};
    uint16_t buf[2][256]={{0}};
    uint32_t ha,hb,stale,extra[PORT_CHANNEL_LIMIT];
    int calls=0;
    port_channels_init(&t);port_event_init(&flags);
    port_qio_init(&svc,read_ok,&calls);
    vms_ast_init(&a,100u);vms_ast_init(&b,200u);
    assert(port_channel_assign(&t,a.pid,read_ok,&calls,&ha)==0);
    stale=ha;
    assert(port_channel_check(&t,ha,a.pid)==0);
    assert(port_channel_check(&t,ha,b.pid)==PORT_CHANNEL_OWNER);
    assert(port_channel_qio_submit(&t,&svc,ha,&req[0],&iosb[0],buf[0],4,1,
                                   &b,done,0,123)==PORT_CHANNEL_OWNER);
    assert(port_channel_qio_submit(&t,&svc,ha,&req[0],&iosb[0],buf[0],4,1,
                                   &a,done,0,123)==0);
    assert(req[0].channel==ha);
    assert(port_channel_deassign(&t,ha,a.pid)==PORT_CHANNEL_BUSY);
    assert(port_channel_qio_cancel(&t,&svc,ha,&req[0],&flags,b.pid)==PORT_CHANNEL_OWNER);
    assert(port_channel_qio_release(&t,&req[0])==PORT_QIO_BUSY);
    assert(port_qio_run_one(&svc,&flags,&a)==1);
    assert(iosb[0].byte_count==512&&buf[0][0]==4);
    assert(port_channel_qio_release(&t,&req[0])==PORT_QIO_BUSY);
    assert(vms_ast_dispatch_one(&a,VMS_MODE_USER)==VMS_AST_OK);
    assert(callback_count==1);
    assert(port_channel_qio_release(&t,&req[0])==0);
    assert(port_channel_deassign(&t,ha,a.pid)==0);
    assert(port_channel_check(&t,stale,a.pid)==PORT_CHANNEL_INVALID);
    assert(port_channel_assign(&t,b.pid,read_ok,&calls,&hb)==0);
    assert(hb!=stale);
    assert(port_channel_check(&t,stale,b.pid)==PORT_CHANNEL_INVALID);
    assert(port_channel_check(&t,hb,b.pid)==0);
    /* Mismatched backend must never be used via the channel. */
    assert(port_channel_assign(&t,a.pid,read_other,0,&ha)==0);
    assert(port_channel_qio_submit(&t,&svc,ha,&req[0],&iosb[0],buf[0],4,0,
                                   &a,0,0,0)==PORT_CHANNEL_BACKEND);
    assert(port_channel_deassign(&t,ha,a.pid)==0);
    /* Multi-owner requests: process B can make progress behind A. */
    assert(port_channel_assign(&t,a.pid,read_ok,&calls,&ha)==0);
    assert(port_channel_qio_submit(&t,&svc,ha,&req[0],&iosb[0],buf[0],5,0,
                                   &a,0,0,0)==0);
    assert(port_channel_qio_submit(&t,&svc,hb,&req[1],&iosb[1],buf[1],6,2,
                                   &b,0,0,0)==0);
    assert(port_qio_run_one(&svc,&flags,&b)==1);
    assert(iosb[1].byte_count==512&&buf[1][0]==6);
    assert(svc.count==1);
    assert(port_channel_qio_release(&t,&req[1])==0);
    assert(port_channel_deassign(&t,hb,b.pid)==0);
    assert(port_qio_run_one(&svc,&flags,&a)==1);
    assert(buf[0][0]==5);
    assert(port_channel_qio_release(&t,&req[0])==0);
    assert(port_channel_deassign(&t,ha,a.pid)==0);
    /* Cancellation requires matching owner and the same handle. */
    assert(port_channel_assign(&t,a.pid,read_ok,&calls,&ha)==0);
    assert(port_channel_qio_submit(&t,&svc,ha,&req[0],&iosb[0],buf[0],7,8,
                                   &a,0,0,0)==0);
    assert(port_channel_qio_cancel(&t,&svc,ha,&req[0],&flags,b.pid)==PORT_CHANNEL_OWNER);
    assert(port_channel_qio_cancel(&t,&svc,ha,&req[0],&flags,a.pid)==0);
    assert(iosb[0].status==PORT_QIO_IOSB_CANCELLED);
    assert(port_channel_qio_release(&t,&req[0])==0);
    assert(port_channel_deassign(&t,ha,a.pid)==0);
    /* All channel slots allocated, then quota. */
    for(unsigned i=0;i<PORT_CHANNEL_LIMIT;i++)
        assert(port_channel_assign(&t,a.pid,read_ok,&calls,&extra[i])==0);
    assert(port_channel_assign(&t,a.pid,read_ok,&calls,&ha)==PORT_CHANNEL_FULL);
    for(unsigned i=0;i<PORT_CHANNEL_LIMIT;i++)
        assert(port_channel_deassign(&t,extra[i],a.pid)==0);
    assert(t.assigned==t.deassigned);
    assert(t.denied==1&&t.peak_outstanding==1&&calls==3);
    puts("M26 owned channel handles, stale protection, mixed processes, deferred QIO PASS");
    return 0;
}
