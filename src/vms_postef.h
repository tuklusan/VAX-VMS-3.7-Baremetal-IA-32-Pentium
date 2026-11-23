#ifndef PORTLAB_VMS_POSTEF_H
#define PORTLAB_VMS_POSTEF_H
#include <stdint.h>
#include "vms_event.h"
#include "vms_pcb.h"

/* Independent IA-32 executive model for the responsibilities of the
 * VAX/VMS POSTEF.MAR source file. No VAX ABI, 4-CPU support or DEC code.
 * The service serializes calls externally. Global EF number 0..127:
 * 0..31 private bank 0, 32..63 private bank 1,
 * 64..95 common bank 2, 96..127 common bank 3.
 */
enum { PORT_POSTEF_PROCS=8, PORT_POSTEF_SYNC_SLOTS=8 };
enum port_postef_rc {
    PORT_POSTEF_WAS_CLEAR=1, PORT_POSTEF_WAS_SET=2,
    PORT_POSTEF_WAIT_BLOCKED=3, PORT_POSTEF_WAIT_IMMEDIATE=4,
    PORT_POSTEF_BAD_PID=-1, PORT_POSTEF_BAD_CLUSTER=-2,
    PORT_POSTEF_UNASSIGNED=-3, PORT_POSTEF_INVALID=-4,
    PORT_POSTEF_SYNC_BACKLOG=-5, PORT_POSTEF_FULL=-6
};
struct port_postef_task {
    struct port_pcb *pcb;
    struct port_event_cluster private_banks[2];
    uint8_t attached_common; /* bit 0 for #2, bit 1 for #3 */
    uint8_t wait_bank;       /* 0..3 or 0xff for no wait */
    uint32_t wakeups;
    uint32_t last_boost_class;
};
struct port_postef_message {
    uint8_t bank;
    uint32_t flags;
    uint32_t sequence;
};
struct port_postef_service {
    struct port_postef_task tasks[PORT_POSTEF_PROCS];
    struct port_event_cluster *common[2];
    /* Local representation of a remote processor common EF replica.
     * This is a queue-backed model, not actual cross-CPU transport. */
    struct port_event_cluster replica[2];
    struct port_postef_message fifo[PORT_POSTEF_SYNC_SLOTS];
    uint8_t head,used;
    uint32_t seq,notifications,delivered,sync_failures,posts,wakes;
};
void port_postef_init(struct port_postef_service *s);
int port_postef_register(struct port_postef_service *s,struct port_pcb *pcb);
int port_postef_unregister(struct port_postef_service *s,uint32_t pid);
int port_postef_bind_common(struct port_postef_service *s,unsigned bank,
                            struct port_event_cluster *events);
int port_postef_attach_common(struct port_postef_service *s,uint32_t pid,unsigned bank);
int port_postef_post(struct port_postef_service *s,uint32_t pid,unsigned ef_number,
                     unsigned priority_class,unsigned *woken);
int port_postef_post_mask(struct port_postef_service *s,uint32_t pid,unsigned bank,
                          uint32_t mask,unsigned priority_class,unsigned *woken);
int port_postef_clear_mask(struct port_postef_service *s,uint32_t pid,
                           unsigned bank,uint32_t mask);
int port_postef_wait(struct port_postef_service *s,uint32_t pid,unsigned bank,
                     uint32_t mask,int all);
int port_postef_drain_one(struct port_postef_service *s);
unsigned port_postef_pending(const struct port_postef_service *s);
/* Same test program compiled for both real IA-32 QEMU guest and native host. */
uint32_t port_postef_selftest(void);
#endif
