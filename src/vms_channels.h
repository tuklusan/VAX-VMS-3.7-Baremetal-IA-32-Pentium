#ifndef PORTLAB_CHANNELS_H
#define PORTLAB_CHANNELS_H
#include <stdint.h>
#include "vms_qio.h"
/* Independent per-process I/O channel table, not original VMS ABI. */
#define PORT_CHANNEL_LIMIT 8u
#define PORT_CHANNEL_OK 0
#define PORT_CHANNEL_INVALID (-1)
#define PORT_CHANNEL_FULL (-2)
#define PORT_CHANNEL_OWNER (-3)
#define PORT_CHANNEL_BUSY (-4)
#define PORT_CHANNEL_BACKEND (-5)
struct port_channel {
    uint32_t owner_pid;
    uint32_t generation;
    uint32_t outstanding;
    struct port_qio_request *requests[PORT_QIO_MAX_QUEUED];
    port_qio_read_backend reader;
    void *context;
    uint8_t used;
};
struct port_channel_table {
    struct port_channel channels[PORT_CHANNEL_LIMIT];
    uint32_t assigned,deassigned,denied,peak_outstanding;
};
void port_channels_init(struct port_channel_table *t);
int port_channel_assign(struct port_channel_table *t,uint32_t pid,
                        port_qio_read_backend reader,void *context,uint32_t *handle);
int port_channel_check(const struct port_channel_table *t,uint32_t handle,
                       uint32_t pid);
int port_channel_deassign(struct port_channel_table *t,uint32_t handle,uint32_t pid);
int port_channel_qio_submit(struct port_channel_table *t,struct port_qio_service *s,
                            uint32_t handle,struct port_qio_request *r,
                            struct port_qio_iosb *iosb,uint16_t buffer[256],
                            uint32_t lba,uint32_t event_mask,
                            const struct vms_ast_process *owner,
                            vms_ast_callback callback,void *context,uint32_t parameter);
int port_channel_qio_cancel(struct port_channel_table *t,
                            struct port_qio_service *s,uint32_t handle,
                            struct port_qio_request *r,
                            struct port_event_cluster *events,uint32_t pid);
int port_channel_qio_release(struct port_channel_table *t,struct port_qio_request *r);
/* Process exit: cancel pending I/O, drop queued ASTs, reclaim all owned
 * channels. Refuses teardown if any callback is actively executing. */
int port_channels_teardown_owner(struct port_channel_table *t,
    struct port_qio_service *s,struct port_event_cluster *events,
    struct vms_ast_process *owner);
#endif
