#ifndef VMS_FILERWIO_H
#define VMS_FILERWIO_H
#include <stdint.h>
/* SYS/SRC/FILERWIO.MAR, FIL$RDWRTLBN. The QIOW returns a service
 * status first, then the IOSB holds the device completion code. */
struct port_filerwio_iosb { uint16_t status; uint16_t count; uint32_t extra; };
struct port_filerwio_service {
    uint32_t (*qiow)(void *ctx,uint32_t channel,uint32_t function,
                     void *buffer,uint32_t bytes,uint32_t lbn,
                     struct port_filerwio_iosb *iosb);
    void *ctx;
};
uint32_t port_filerwio_rdwr_lbn(const struct port_filerwio_service *service,
                               uint32_t channel,uint32_t lbn,void *buffer,
                               uint32_t function,uint32_t bytes);
#endif
