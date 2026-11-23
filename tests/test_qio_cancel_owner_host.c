#include <stdint.h>
#include <assert.h>
#include <stdio.h>
#include "vms_channels.h"
static unsigned reads;
static int read_block(void *context,uint32_t lba,uint16_t out[256]){
    (void)context;(void)lba;reads++;
    for(unsigned i=0;i<256;i++)out[i]=0;
    return 0;
}
int main(void) {
    struct port_channel_table table;
    struct port_qio_service service;
    struct port_event_cluster events;
    struct vms_ast_process owner;
    struct port_qio_request req={0};
    struct port_qio_iosb iosb={0};
    uint16_t sector[256]={0};uint32_t h=0;
    port_channels_init(&table);
    port_qio_init(&service,read_block,0);
    port_event_init(&events);
    vms_ast_init(&owner,42);
    assert(port_channel_assign(&table,42,read_block,0,&h)==0);
    assert(port_channel_qio_submit(&table,&service,h,&req,&iosb,sector,1,0,
                                   &owner,0,0,0)==0);
    assert(port_channel_deassign(&table,h,42)==PORT_CHANNEL_BUSY);
    assert(port_channel_qio_cancel(&table,&service,h,&req,&events,43)==PORT_CHANNEL_OWNER);
    assert(port_channel_qio_cancel(&table,&service,h+1,&req,&events,42)!=0);
    assert(port_channel_qio_cancel(&table,&service,h,&req,&events,42)==0);
    assert(iosb.status==PORT_QIO_IOSB_CANCELLED);
    assert(reads==0 && service.count==0);
    assert(port_channel_qio_release(&table,&req)==0);
    assert(port_channel_deassign(&table,h,42)==0);
    assert(port_channel_deassign(&table,h,42)==PORT_CHANNEL_INVALID);
    assert(port_channel_qio_release(&table,&req)!=0);
    puts("M31 owned QIO cancel, release, stale and wrong-PID denial PASS");
    return 0;
}
