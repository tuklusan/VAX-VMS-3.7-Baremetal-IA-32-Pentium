#include "vms_qio_post.h"
/* NLDRIVER.MAR READ/WRITE to SYSQIOREQ.MAR EXE$FINISHIOC: a real
 * source-driver completion through the original-layout IRP and queue.
 * No $QIO channel assignment or IOPOST AST dispatch is implied. */
uint32_t port_v37_nl_fdt_post(struct port_v37_qio_request *request){
    if(!request||!request->irp||request->irp_size<PORT_V37_IRP_SIZE||
       !request->ucb||request->ucb_size<PORT_V37_UCB_SIZE||
       !request->nl_ddb_va || (request->nl_ddb_va&3u) ||
       port_v37_read32(request->ucb+36u)!=request->nl_ddb_va ||
       port_v37_read16(request->ucb+72u)!=0u ||
       !(port_v37_read16(request->ucb+88u)&PORT_V37_UCB_ONLINE))
        return PORT_V37_QIO_BAD;
    const uint16_t function=port_v37_read16(request->irp+PORT_V37_IRP_FUNC);
    return port_nl_driver_io(function,request,port_v37_qio_finishioc);
}
