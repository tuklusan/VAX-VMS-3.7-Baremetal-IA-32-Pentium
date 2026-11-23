#ifndef PORTLAB_QIO_H
#define PORTLAB_QIO_H
#include <stdint.h>
#include "vms_ast.h"
#include "vms_event.h"
/* VMS-inspired IO$_READLBLK-style single-block request model.
 * This is independent test code, NOT the original VMS SYS$QIO ABI. */
#define PORT_QIO_MAX_QUEUED 4u
#define PORT_QIO_IOSB_SUCCESS 1u
#define PORT_QIO_IOSB_IOERROR 2u
#define PORT_QIO_IOSB_CANCELLED 4u
#define PORT_QIO_OK 0
#define PORT_QIO_INVALID (-1)
#define PORT_QIO_BUSY (-2)
#define PORT_QIO_NO_QUOTA (-3)
typedef int (*port_qio_read_backend)(void *ctx,uint32_t lba,uint16_t dst[256]);
struct port_qio_iosb {
    uint16_t status;
    uint16_t byte_count;
    uint32_t detail;
};
struct port_qio_request {
    struct port_qio_request *next;
    struct port_qio_iosb *iosb;
    uint16_t *sector_buffer;
    uint32_t lba, pid, event_mask, channel;
    vms_ast_callback completion;
    void *completion_context;
    uint32_t completion_parameter;
    struct vms_ast_request ast;
    uint8_t state; /* 0=free 1=queued 2=completed */
};
struct port_qio_service {
    struct port_qio_request *head,*tail;
    unsigned count;
    port_qio_read_backend reader;
    void *context;
    uint32_t submitted,completed,failed,cancelled,ast_dropped;
};
void port_qio_init(struct port_qio_service *s,port_qio_read_backend reader,
                   void *context);
int port_qio_submit(struct port_qio_service *s,struct port_qio_request *r,
                     struct port_qio_iosb *iosb,uint16_t sector_buffer[256],
                     uint32_t lba,uint32_t event_mask,
                     const struct vms_ast_process *owner,
                     vms_ast_callback completion,void *completion_context,
                     uint32_t completion_parameter);
/* Deferred I/O completion occurs at the kernel safe point, not IRQ time. */
int port_qio_run_one(struct port_qio_service *s,struct port_event_cluster *events,
                     struct vms_ast_process *owner);
int port_qio_cancel(struct port_qio_service *s,struct port_qio_request *r,
                    struct port_event_cluster *events);
int port_qio_release(struct port_qio_request *r);
#endif
