#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include "vms_qio.h"
struct backend {int calls,fail;uint32_t lba_seen;};
static int rd(void *ctx,uint32_t lba,uint16_t buf[256]) {
    struct backend *b=ctx;
    ++b->calls;b->lba_seen=lba;
    if(b->fail)return -17;
    for(unsigned i=0;i<256;i++)buf[i]=(uint16_t)(lba+i);
    return 0;
}
struct callback_info {uint32_t count,last;};
static void ast(void *ctx,uint32_t value){struct callback_info *c=ctx;++c->count;c->last=value;}
int main(void) {
    struct port_qio_service service;
    struct port_event_cluster flags;
    struct vms_ast_process process;
    struct port_qio_request req[6]={{0}};
    struct port_qio_iosb iosb[6]={{0}};
    uint16_t buffers[6][256]={{0}};
    struct backend backend={0};
    struct callback_info called={0};
    port_event_init(&flags);vms_ast_init(&process,0x370001u);
    port_qio_init(&service,rd,&backend);
    assert(port_qio_submit(&service,&req[0],&iosb[0],buffers[0],6,0x2,
           &process,ast,&called,77)==0);
    assert(iosb[0].status==0&&req[0].state==1&&service.count==1);
    assert(port_qio_submit(&service,&req[0],&iosb[0],buffers[0],7,1,
           &process,0,0,0)==PORT_QIO_INVALID);
    assert(port_qio_run_one(&service,&flags,&process)==1);
    assert(iosb[0].status==PORT_QIO_IOSB_SUCCESS&&iosb[0].byte_count==512);
    assert(port_event_satisfied(&flags,2,1)&&buffers[0][0]==6);
    assert(process.queued==1&&called.count==0);
    assert(port_qio_release(&req[0])==PORT_QIO_BUSY);
    assert(vms_ast_dispatch_one(&process,VMS_MODE_USER)==VMS_AST_OK);
    assert(called.count==1&&called.last==77);
    assert(port_qio_release(&req[0])==PORT_QIO_OK);
    assert(port_qio_run_one(&service,&flags,&process)==0);
    for(unsigned i=0;i<4;i++) {
        assert(port_qio_submit(&service,&req[i],&iosb[i],buffers[i],i,0,
              &process,0,0,0)==0);
    }
    assert(port_qio_submit(&service,&req[4],&iosb[4],buffers[4],4,0,
           &process,0,0,0)==PORT_QIO_NO_QUOTA);
    assert(port_qio_cancel(&service,&req[2],&flags)==0);
    assert(iosb[2].status==PORT_QIO_IOSB_CANCELLED);
    assert(port_qio_release(&req[2])==0);
    assert(service.count==3);
    backend.fail=1;
    assert(port_qio_run_one(&service,&flags,&process)==1);
    assert(iosb[0].status==PORT_QIO_IOSB_IOERROR&&iosb[0].detail==(uint32_t)-17);
    assert(iosb[0].byte_count==0);
    assert(port_qio_release(&req[0])==0);
    backend.fail=0;
    assert(port_qio_run_one(&service,&flags,&process)==1);
    assert(backend.lba_seen==1);
    assert(port_qio_run_one(&service,&flags,&process)==1);
    assert(backend.lba_seen==3&&service.count==0);
    assert(port_qio_release(&req[1])==0);
    assert(port_qio_release(&req[3])==0);
    assert(service.cancelled==1&&service.failed==1);
    assert(port_qio_submit(&service,&req[0],&iosb[0],buffers[0],0,3,
           &process,0,0,0)==PORT_QIO_INVALID);
    assert(port_qio_submit(&service,&req[0],&iosb[0],buffers[0],0,4,
           &process,0,0,0)==0);
    puts("M25 deferred block QIO + IOSB/event flag/user AST/error/cancel PASS");
    return 0;
}
