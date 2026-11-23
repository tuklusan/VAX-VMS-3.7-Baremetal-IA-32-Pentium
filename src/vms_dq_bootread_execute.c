#include "vms_dq_bootread.h"
/* Complete one READPBLK of the original boot disk, using real IA32 ATA
 * only. No hidden fake disk, user-mode $QIO, RB730 emulation or write. */
uint32_t port_dq_bootread_execute_physical(const struct port_dq_bootread_request *q,
                                  struct port_dq_bootread_result *out,
                                  uint32_t requested_lbn){
    if(!out)return PORT_DQ_BOOTREAD_BAD;
    uint32_t st=port_dq_bootread_preflight(q);
    if(st!=PORT_DQ_BOOTREAD_OK)return st;
    uint32_t lbn=0u;uint16_t function=0u;
    st=port_dq_bootread_decode_physical(q,&lbn,&function,requested_lbn);
    if(st!=PORT_DQ_BOOTREAD_OK)return st;
    if(function!=PORT_DQ_READPBLK||lbn!=requested_lbn)
        return PORT_DQ_BOOTREAD_UNBOUND;
    uint16_t scratch[256];
    if(q->read_sector(q->read_context,lbn,scratch)!=0)
        return PORT_DQ_BOOTREAD_FAILED;
    return port_dq_bootread_commit(q,scratch,out);
}

uint32_t port_dq_bootread_execute(const struct port_dq_bootread_request *q,
                                  struct port_dq_bootread_result *out){
    return port_dq_bootread_execute_physical(q,out,PORT_DQ_BOOTREAD_LBN);
}
