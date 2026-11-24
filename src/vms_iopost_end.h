#ifndef VMS37_IOPOST_END_H
#define VMS37_IOPOST_END_H
#include <stddef.h>
#include <stdint.h>
#include "vms_qio_post.h"
#include "vms_v37_ccb.h"
#include "vms_v37_phd.h"
#include "vms_ini_procvec.h"
#include "vms_v37_pcb.h"
/* SYS/SRC/IOCIOPOST.MAR 115-165. The original kernel end action
 * recognises a negative VAX system-space address in IRP$L_PID; native
 * IA-32 uses the checked supervisor-only SYS.EXE mapping instead.
 * Optional callback path accepts retained SYSQIO state, but no running
 * process/IOSB/scheduler binding or ordinary $QIO dispatch exists yet. */
enum {PORT_V37_IOPOST_OK=1u,PORT_V37_IOPOST_BAD=2u,
      PORT_V37_IOPOST_EMPTY=4u,PORT_V37_IOPOST_DEFER=6u,
      PORT_V37_IOPOST_CORRUPT=8u,PORT_V37_IOPOST_FULL=10u,
      PORT_V37_IOPOST_CCB_DEFER=12u,
      PORT_V37_IOPOST_DIRPOST_DEFER=14u,
      PORT_V37_IOPOST_UNLOCK_DEFER=16u,
      PORT_V37_IOPOST_PROCESS_DEFER=18u,
      PORT_V37_IOPOST_IRPE_DEFER=20u,
      PORT_V37_IOPOST_LOG_CAP=2u,PORT_V37_IRP_PID=12u,
      PORT_V37_IRP_AST=16u,PORT_V37_IRP_WIND=24u,
      PORT_V37_IRP_EXTEND=84u,
      PORT_V37_IRP_IOSB=36u,PORT_V37_IRP_STS=42u,
      PORT_V37_IRPE_EXTEND=84u,PORT_V37_IRPE_STS=42u,
      PORT_V37_IRPE_BYTES=88u,PORT_V37_IOPOST_MAX_IRPES=32u};
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
typedef int (*port_v37_iopost_user_qio_validate_fn)(void *,uint32_t,
    uint32_t,uint32_t,uint8_t);
typedef uint32_t (*port_v37_iopost_user_qio_complete_fn)(void *,uint32_t,
    uint32_t,uint32_t,uint8_t,uint32_t,uint32_t);
typedef void (*port_v37_iopost_event_fn)(void *,uint32_t,uint8_t);
typedef void (*port_v37_iopost_packet_fn)(void *,uint32_t,uint32_t);
typedef int (*port_v37_iopost_resolve_irpe_fn)(void *,uint32_t,
    const uint8_t **,size_t *);
struct port_v37_iopost_irpe_plan {
    uint32_t address[PORT_V37_IOPOST_MAX_IRPES];
    uint8_t count;
};
uint32_t port_v37_iopost_irpe_plan(const uint8_t *,size_t,
    port_v37_iopost_resolve_irpe_fn,void *,uint8_t,
    struct port_v37_iopost_irpe_plan *);
uint32_t port_v37_iopost_irpe_release(
    const struct port_v37_iopost_irpe_plan *,port_v37_iopost_packet_fn,
    void *,uint32_t);
struct port_v37_iopost_dirpost_ops {
    void *context;
    port_v37_iopost_account_fn account_io;
    struct port_v37_iopost_ccb_ops ccb;
    uint8_t *ccb_base;
    size_t ccb_preceding_bytes;
    port_v37_iopost_iosb_fn store_iosb_if_writable;
    /* Native prepared SYS$QIO path: validate retained IOSB/buffer ownership
     * before DIRPOST mutates accounting, then write IOSB and release pins. */
    port_v37_iopost_user_qio_validate_fn validate_user_qio;
    port_v37_iopost_user_qio_complete_fn complete_user_qio;
    port_v37_iopost_event_fn post_event;
    port_v37_iopost_packet_fn queue_ast;
    port_v37_iopost_packet_fn free_irp;
    port_v37_iopost_resolve_irpe_fn resolve_irpe;
    port_v37_iopost_packet_fn free_irpe;
    uint8_t maximum_extensions;
};
/* IOCIOPOST.MAR DIRPOST's process-context common completion through
 * SCH$POSTEF and the SCH$QAST/deallocation choice. Callbacks bind the live
 * PHD, guarded IOSB store, event/AST queues and IRP lookaside. */
uint32_t port_v37_iopost_dirpost(uint8_t *,size_t,uint32_t,
    const struct port_v37_iopost_dirpost_ops *);
typedef void (*port_v37_iopost_unlock_pages_fn)(void *,uint32_t,uint32_t);
struct port_v37_iopost_unlock_ops {
    void *context;
    port_v37_iopost_resolve_irpe_fn resolve_irpe;
    port_v37_iopost_unlock_pages_fn unlock_pages;
    uint8_t maximum_extensions;
};
/* IOCIOPOST.MAR DIRECT I/O path: release locked user-buffer pages, including
 * both regions recorded by each IRPE. PAGIO/SWAPIO and segmented virtual I/O
 * require their separate source paths and are rejected without mutation. */
uint32_t port_v37_iopost_unlock_direct(uint8_t *,size_t,
    const struct port_v37_iopost_unlock_ops *);
/* IOCIOPOST.MAR DIRPOST PHD$L_DIOCNT/BIOCNT accounting, before CCB posting. */
uint32_t port_v37_iopost_account_phd(uint8_t *,size_t,uint8_t);
typedef int (*port_v37_iopost_resolve_pcb_fn)(void *,uint32_t,
    uint8_t **,size_t *);
typedef void (*port_v37_iopost_queue_kast_fn)(void *,uint32_t,uint32_t,
    uint32_t);
struct port_v37_iopost_process_ops {
    void *context;
    const struct port_ini_procvec *process_vector;
    port_v37_iopost_resolve_pcb_fn resolve_pcb;
    port_v37_iopost_resolve_irpe_fn resolve_irpe;
    port_v37_iopost_unlock_pages_fn unlock_pages;
    port_v37_iopost_queue_kast_fn queue_kast;
    uint8_t maximum_extensions;
};
/* IOCIOPOST.MAR process-vector/direct-I/O portion: resolve positive PID,
 * charge PCB$W_DIOCNT, unlock direct buffers, and queue the completion KAST.
 * Buffered, paging, swap and segmented-virtual requests remain unhandled. */
uint32_t port_v37_iopost_process_direct(uint8_t *,size_t,uint32_t,
    const struct port_v37_iopost_process_ops *);
#endif
