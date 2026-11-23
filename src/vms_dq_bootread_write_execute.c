#include "vms_dq_bootread.h"
#include "vms_dq_ioc_write.h"
static struct port_dq_ioc_write_queue private_write_queue;
static uint32_t private_write_active;
static uint32_t write_sector_status(void *context,uint32_t lbn,
    const uint8_t data[512]){
    const struct port_dq_bootread_request *q=context;
    return q&&q->write_sector&&q->write_sector(q->read_context,lbn,
        (const uint16_t *)(const void *)data)==0?PORT_DQ_IOCW_OK:
                                                   PORT_DQ_IOCW_IO;
}
/* Physical WRITEPBLK action for the private native ATA backend. The IRP is
 * copied into the source-shaped IOC FIFO and completed only after the ATA
 * backend reports command completion. This private queue is synchronous. */
uint32_t port_dq_bootwrite_execute_physical(
    const struct port_dq_bootread_request *q,
    struct port_dq_bootread_result *out,uint32_t requested_lbn){
    if(!out)return PORT_DQ_BOOTREAD_BAD;
    uint32_t st=port_dq_bootread_preflight(q);
    if(st!=PORT_DQ_BOOTREAD_OK)return st;
    uint32_t lbn=0u;uint16_t function=0u;
    st=port_dq_bootread_decode_physical(q,&lbn,&function,requested_lbn);
    if(st!=PORT_DQ_BOOTREAD_OK)return st;
    if(function!=PORT_DQ_WRITEPBLK||lbn!=requested_lbn)
        return PORT_DQ_BOOTREAD_UNBOUND;
    if(private_write_active)return PORT_DQ_BOOTREAD_FAILED;
    private_write_active=1u;
    struct port_dq_ioc_write_iosb iosb={0};
    const uint32_t queued=port_dq_ioc_write_init(&private_write_queue,
        write_sector_status,(void *)q)==PORT_DQ_IOCW_OK&&
        port_dq_ioc_write_prepare(&private_write_queue,1u,lbn,
          (const uint8_t *)(const void *)q->sector,q->sector_bytes)==
              PORT_DQ_IOCW_OK&&
        port_dq_ioc_write_enqueue(&private_write_queue,1u)==PORT_DQ_IOCW_OK&&
        port_dq_ioc_write_service(&private_write_queue,&iosb)==
              PORT_DQ_IOCW_OK&&
        iosb.tag==1u&&iosb.status==PORT_DQ_IOCW_OK&&iosb.bytes==512u&&
        private_write_queue.completed==1u;
    private_write_active=0u;
    if(!queued)
        return PORT_DQ_BOOTREAD_FAILED;
    return port_dq_bootwrite_commit(q,out);
}
