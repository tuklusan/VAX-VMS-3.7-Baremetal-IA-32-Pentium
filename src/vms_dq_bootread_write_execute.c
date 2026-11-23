#include "vms_dq_bootread.h"
/* Physical WRITEPBLK action for the private native ATA backend. The IRP is
 * completed only after the low-level controller reports command completion. */
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
    if(q->write_sector(q->read_context,lbn,q->sector)!=0)
        return PORT_DQ_BOOTREAD_FAILED;
    return port_dq_bootwrite_commit(q,out);
}
