#include <stdint.h>
#include <assert.h>
#include <stdio.h>
#include "vms_channels.h"
static unsigned reads;
static int backend(void *context,uint32_t lba,uint16_t b[256]){
    (void)context;(void)lba;reads++;
    for(unsigned i=0;i<256;i++)b[i]=1;
    return 0;
}
static void callback(void *context,uint32_t param){(void)context;(void)param;}
int main(void){
    struct port_channel_table channels;
    struct port_qio_service service;
    struct port_event_cluster events;
    struct vms_ast_process a,b;
    struct port_qio_request req[4]={{0}};
    struct port_qio_iosb iosb[4]={{0}};
    uint16_t data[4][256]={{0}};
    uint32_t ha,hb;
    port_channels_init(&channels);port_qio_init(&service,backend,0);
    port_event_init(&events);vms_ast_init(&a,100);vms_ast_init(&b,200);
    assert(!port_channel_assign(&channels,a.pid,backend,0,&ha));
    assert(!port_channel_assign(&channels,b.pid,backend,0,&hb));
    assert(!port_channel_qio_submit(&channels,&service,ha,&req[0],&iosb[0],data[0],1,0,&a,callback,0,9));
    assert(!port_channel_qio_submit(&channels,&service,ha,&req[1],&iosb[1],data[1],2,0,&a,0,0,0));
    assert(!port_channel_qio_submit(&channels,&service,hb,&req[2],&iosb[2],data[2],3,0,&b,0,0,0));
    assert(port_qio_run_one(&service,&events,&a)==1);
    assert(a.queued==1&&req[0].state==2&&service.count==2);
    assert(!port_channels_teardown_owner(&channels,&service,&events,&a));
    assert(a.queued==0 && req[0].state==0 && req[1].state==0);
    assert(iosb[1].status==PORT_QIO_IOSB_CANCELLED);
    assert(service.count==1 && port_channel_check(&channels,ha,a.pid)==PORT_CHANNEL_INVALID);
    assert(port_channel_check(&channels,hb,b.pid)==0);
    assert(port_qio_run_one(&service,&events,&b)==1);
    assert(!port_channel_qio_release(&channels,&req[2]));
    assert(!port_channel_deassign(&channels,hb,b.pid));
    assert(reads==2&&channels.assigned==channels.deassigned);
    /* Running user AST cannot be reclaimed: reject teardown atomically. */
    assert(!port_channel_assign(&channels,a.pid,backend,0,&ha));
    assert(!port_channel_qio_submit(&channels,&service,ha,&req[3],&iosb[3],data[3],4,0,&a,callback,0,11));
    assert(port_qio_run_one(&service,&events,&a)==1);
    struct vms_ast_request *active=0;
    assert(vms_ast_take(&a,VMS_MODE_USER,&active)==VMS_AST_OK);
    assert(active==&req[3].ast && req[3].ast.state==2);
    assert(port_channels_teardown_owner(&channels,&service,&events,&a)==PORT_CHANNEL_BUSY);
    assert(port_channel_check(&channels,ha,a.pid)==0 && req[3].state==2);
    assert(vms_ast_complete(&a,active)==VMS_AST_OK);
    assert(!port_channels_teardown_owner(&channels,&service,&events,&a));
    assert(req[3].state==0 && channels.assigned==channels.deassigned);
    puts("M32 owner teardown, queued AST cancel, active callback atomic refusal PASS");
    return 0;
}
