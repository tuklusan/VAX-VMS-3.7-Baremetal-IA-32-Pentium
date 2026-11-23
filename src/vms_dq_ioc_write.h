#ifndef VMS37_DQ_IOC_WRITE_H
#define VMS37_DQ_IOC_WRITE_H
#include <stddef.h>
#include <stdint.h>

enum { PORT_DQ_IOCW_DEPTH=8u,PORT_DQ_IOCW_OK=1u,PORT_DQ_IOCW_BAD=2u,
       PORT_DQ_IOCW_FULL=4u,PORT_DQ_IOCW_IO=6u,
       PORT_DQ_IOCW_PREPARED=1u,PORT_DQ_IOCW_QUEUED=2u,
       PORT_DQ_IOCW_ACTIVE=3u };
typedef uint32_t (*port_dq_ioc_write_backend)(void *,uint32_t,
                                               const uint8_t [512]);
struct port_dq_ioc_write_packet {
    uint32_t tag,lbn,order;
    uint8_t state;
    uint8_t reserved[3];
    uint8_t payload[512];
};
struct port_dq_ioc_write_queue {
    struct port_dq_ioc_write_packet packets[PORT_DQ_IOCW_DEPTH];
    port_dq_ioc_write_backend backend;
    void *context;
    uint32_t order,submitted,completed,failed;
};
struct port_dq_ioc_write_iosb { uint32_t tag;uint16_t status,bytes; };
uint32_t port_dq_ioc_write_init(struct port_dq_ioc_write_queue *,
    port_dq_ioc_write_backend,void *);
uint32_t port_dq_ioc_write_prepare(struct port_dq_ioc_write_queue *,
    uint32_t,uint32_t,const uint8_t *,size_t);
uint32_t port_dq_ioc_write_enqueue(struct port_dq_ioc_write_queue *,uint32_t);
uint32_t port_dq_ioc_write_service(struct port_dq_ioc_write_queue *,
    struct port_dq_ioc_write_iosb *);
#endif
