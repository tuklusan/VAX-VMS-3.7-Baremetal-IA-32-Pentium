#include "vms_filerwio.h"
uint32_t port_filerwio_rdwr_lbn(const struct port_filerwio_service *service,
                               uint32_t channel,uint32_t lbn,void *buffer,
                               uint32_t function,uint32_t bytes) {
    if(!service || !service->qiow || (!buffer && bytes))return 0u;
    /* FILERWIO.MAR allocates its eight-byte IOSB on the stack, issues
     * $QIOW_S and returns the IOSB word on successful service return. */
    struct port_filerwio_iosb iosb={0,0,0};
    uint32_t status=service->qiow(service->ctx,channel,function,buffer,
                                  bytes,lbn,&iosb);
    if(!(status & 1u))return status;
    return (uint32_t)iosb.status;
}
