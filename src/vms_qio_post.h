#ifndef VMS37_QIO_POST_H
#define VMS37_QIO_POST_H
#include <stdint.h>
#include <stddef.h>
#include "vms_ini_devicedb.h"
#include "vms_sys85.h"
/* SHRLIB/SYSDEF.MDL CDRP defines the IRP at negative offsets through ARB
 * (+88), then its positive fork-queue fields through CDRP$L_LENGTH at +136.
 * That is IRP$C_LENGTH; SYSBOOT rounds it to a 144-byte packet stride.
 * Status longwords are at +56 and +60. The
 * IOC$GL_PSFL/PSBL doubly-linked queue follows VAX INSQUE semantics.
 * This is *not* a public $QIO service or an I/O postprocess AST. */
enum { PORT_V37_IRP_SIZE=136u, PORT_V37_IRP_POOL_STRIDE=144u,
       PORT_V37_IRP_PREFIX_SIZE=64u,
       PORT_V37_IRP_ARB=88u, PORT_V37_IRP_FUNC=32u,
       PORT_V37_IRP_UCB=28u, PORT_V37_IRP_MEDIA=56u,
       PORT_V37_QIO_IRP_IOSB=36u, PORT_V37_IRP_RMOD=11u,
       PORT_V37_IRP_BCNT=52u,PORT_V37_IRP_BOFF=48u,
       PORT_V37_IRP_IOST2=60u, PORT_V37_UCB_OPCNT=96u,
       PORT_V37_IOPOST_IPL=4u, PORT_V37_QIO_OK=1u,
       PORT_V37_QIO_BAD=2u, PORT_V37_QIO_FULL=4u,
       PORT_V37_QIO_DUP=6u, PORT_V37_QIO_CORRUPT=8u };
struct port_v37_iopost_queue {
    uint32_t first,last; /* same shape as IOC$GL_PSFL/IOC$GL_PSBL */
    uint32_t sentinel,first_irp,packet_stride,packet_capacity,queued;
    uint8_t *bytes;
};
struct port_v37_qio_request {
    struct port_v37_iopost_queue *post;
    uint8_t *irp,*ucb;
    uint32_t irp_va,ucb_va,nl_ddb_va;
    size_t irp_size,ucb_size;
    void (*softint)(void *,uint8_t);
    void *softint_arg;
};
uint32_t port_v37_post_init(struct port_v37_iopost_queue *,
    uint8_t *,size_t,uint32_t,uint32_t,uint32_t);
uint32_t port_v37_post_validate(const struct port_v37_iopost_queue *);
uint32_t port_v37_post_append(struct port_v37_iopost_queue *,uint32_t);
uint32_t port_v37_irp_prepare(uint8_t *,size_t,uint32_t,uint32_t,uint16_t);
uint32_t port_v37_qio_finishio(struct port_v37_qio_request *,uint32_t,uint32_t);
uint32_t port_v37_qio_finishioc(void *,uint32_t);
uint32_t port_v37_qio_abortio(struct port_v37_qio_request *,int16_t *);
uint32_t port_v37_nl_fdt_post(struct port_v37_qio_request *);
#endif
