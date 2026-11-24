#ifndef VMS37_IOPOST_END_H
#define VMS37_IOPOST_END_H
#include <stddef.h>
#include <stdint.h>
#include "vms_qio_post.h"
#include "vms_v37_ccb.h"
/* SYS/SRC/IOCIOPOST.MAR 115-165. The original kernel end action
 * recognises a negative VAX system-space address in IRP$L_PID; native
 * IA-32 uses the checked supervisor-only SYS.EXE mapping instead.
 * No process/IOSB, special kernel AST, or ordinary $QIO dispatch yet. */
enum {PORT_V37_IOPOST_OK=1u,PORT_V37_IOPOST_BAD=2u,
      PORT_V37_IOPOST_EMPTY=4u,PORT_V37_IOPOST_DEFER=6u,
      PORT_V37_IOPOST_CORRUPT=8u,PORT_V37_IOPOST_FULL=10u,
      PORT_V37_IOPOST_CCB_DEFER=12u,
      PORT_V37_IOPOST_DIRPOST_DEFER=14u,
      PORT_V37_IOPOST_LOG_CAP=2u,PORT_V37_IRP_PID=12u,
      PORT_V37_IRP_AST=16u,PORT_V37_IRP_WIND=24u,
      PORT_V37_IRP_IOSB=36u,PORT_V37_IRP_STS=42u};
struct port_v37_iopost_endlog {
    uint32_t completed;
    uint32_t irp_va[PORT_V37_IOPOST_LOG_CAP];
    uint32_t status[PORT_V37_IOPOST_LOG_CAP];
    uint32_t secondary[PORT_V37_IOPOST_LOG_CAP];
};
uint32_t port_v37_iopost_peek(const struct port_v37_iopost_queue *,
    uint32_t *,const uint8_t **);
uint32_t port_v37_iopost_validate(const struct port_v37_iopost_queue *,
    uint32_t,uint32_t,uint32_t);
uint32_t port_v37_iopost_pop(struct port_v37_iopost_queue *,uint32_t *);
uint32_t port_v37_iopost_kernel_end(struct port_v37_iopost_endlog *,
    uint32_t,uint32_t,uint32_t);
uint32_t port_v37_iopost_service(struct port_v37_iopost_queue *,
    uint32_t,uint32_t,uint32_t,struct port_v37_iopost_endlog *);
uint32_t port_v37_iopost_drain(struct port_v37_iopost_queue *,
    uint32_t,uint32_t,uint32_t,struct port_v37_iopost_endlog *,uint32_t);
typedef void (*port_v37_iopost_wake_acp_fn)(void *,uint32_t,uint32_t);
struct port_v37_iopost_ccb_ops {
    void *context;
    port_v37_iopost_wake_acp_fn wake_acp;
};
/* IOCIOPOST.MAR DIRPOST 990-1000. channel_negative is IRP$W_CHAN as
 * stored by SYSQIOREQ; a queued deaccess packet is handed to IOC$WAKACP. */
uint32_t port_v37_iopost_ccb_complete(uint8_t *,size_t,uint16_t,
    const struct port_v37_iopost_ccb_ops *,uint16_t *,uint32_t *);
typedef void (*port_v37_iopost_account_fn)(void *,uint32_t,uint8_t);
typedef void (*port_v37_iopost_iosb_fn)(void *,uint32_t,uint8_t,
    uint32_t,uint32_t);
typedef void (*port_v37_iopost_event_fn)(void *,uint32_t,uint8_t);
typedef void (*port_v37_iopost_packet_fn)(void *,uint32_t,uint32_t);
struct port_v37_iopost_dirpost_ops {
    void *context;
    port_v37_iopost_account_fn account_io;
    struct port_v37_iopost_ccb_ops ccb;
    uint8_t *ccb_base;
    size_t ccb_preceding_bytes;
    port_v37_iopost_iosb_fn store_iosb_if_writable;
    port_v37_iopost_event_fn post_event;
    port_v37_iopost_packet_fn queue_ast;
    port_v37_iopost_packet_fn free_irp;
};
/* IOCIOPOST.MAR DIRPOST's process-context common completion through
 * SCH$POSTEF and the SCH$QAST/deallocation choice. Callbacks bind the live
 * PHD, guarded IOSB store, event/AST queues and IRP lookaside. */
uint32_t port_v37_iopost_dirpost(uint8_t *,size_t,uint32_t,
    const struct port_v37_iopost_dirpost_ops *);
#endif
