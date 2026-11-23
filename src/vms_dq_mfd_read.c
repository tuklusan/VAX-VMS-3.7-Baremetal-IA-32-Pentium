#include "vms_dq_mfd_header.h"
/* Fourth privileged source FIL$READVBN operation; no ACP-mediated QIO. */
uint32_t port_dq_mfd_read(const struct port_dq_bootread_request *q,
                          const uint16_t home[256],const uint16_t index[256],
                          struct port_dq_bootread_result *io,
                          struct port_dq_mfd_result *out){
    if(!q||!home||!index||!io||!out||!q->irp)return PORT_DQ_MFD_BAD;
    uint32_t vbn=0u,lbn=0u;
    uint32_t st=port_dq_mfd_location((const uint8_t *)home,
                        (const uint8_t *)index,&vbn,&lbn);
    if(st!=PORT_DQ_MFD_OK)return st;
    if(port_v37_read32(q->irp+PORT_V37_IRP_MEDIA)!=lbn)
        return PORT_DQ_MFD_BAD;
    st=port_dq_bootread_execute_physical(q,io,lbn);
    if(st!=PORT_DQ_BOOTREAD_OK)return PORT_DQ_MFD_IO;
    struct port_dq_mfd_result tmp={0};
    st=port_dq_mfd_validate(q->sector,&tmp);
    if(st!=PORT_DQ_MFD_OK)return st;
    tmp.vbn=vbn;tmp.lbn=lbn;tmp.io_count=io->ucb_io_count;
    *out=tmp;
    return PORT_DQ_MFD_OK;
}
