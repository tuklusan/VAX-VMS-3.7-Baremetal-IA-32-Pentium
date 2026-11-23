/* Native IA-32 executive event posting. Original implementation, with no
 * historical DEC source or VAX machine code. All calls are serialized.
 */
#include "vms_postef.h"

static struct port_postef_task *lookup(struct port_postef_service *s,uint32_t pid) {
    if (!s || !pid) return 0;
    for (unsigned i=0;i<PORT_POSTEF_PROCS;i++)
        if (s->tasks[i].pcb && s->tasks[i].pcb->pid==pid) return &s->tasks[i];
    return 0;
}
void port_postef_init(struct port_postef_service *s) {
    if (!s) return;
    for (unsigned i=0;i<PORT_POSTEF_PROCS;i++) {
        s->tasks[i].pcb=0;
        s->tasks[i].attached_common=0;
        s->tasks[i].wait_bank=0xff;
        s->tasks[i].wakeups=s->tasks[i].last_boost_class=0;
        for (unsigned j=0;j<2;j++) port_event_init(&s->tasks[i].private_banks[j]);
    }
    for (unsigned i=0;i<2;i++) {
        s->common[i]=0;
        port_event_init(&s->replica[i]);
    }
    s->head=s->used=0;
    s->seq=s->notifications=s->delivered=s->sync_failures=s->posts=s->wakes=0;
}
int port_postef_register(struct port_postef_service *s,struct port_pcb *pcb) {
    if (!s || !pcb || !pcb->pid || lookup(s,pcb->pid)) return PORT_POSTEF_INVALID;
    for (unsigned i=0;i<PORT_POSTEF_PROCS;i++) {
        struct port_postef_task *t=&s->tasks[i];
        if (!t->pcb) {
            t->pcb=pcb;t->attached_common=0;t->wait_bank=0xff;
            t->wakeups=t->last_boost_class=0;
            for(unsigned j=0;j<2;j++)port_event_init(&t->private_banks[j]);
            return 0;
        }
    }
    return PORT_POSTEF_FULL;
}
int port_postef_unregister(struct port_postef_service *s,uint32_t pid) {
    struct port_postef_task *t=lookup(s,pid);
    if (!t) return PORT_POSTEF_BAD_PID;
    if (t->pcb->state==PORT_PCB_WAIT_EF) return PORT_POSTEF_INVALID;
    t->pcb=0;t->attached_common=0;t->wait_bank=0xff;
    return 0;
}
int port_postef_bind_common(struct port_postef_service *s,unsigned bank,
                            struct port_event_cluster *events) {
    if (!s || bank<2 || bank>3) return PORT_POSTEF_BAD_CLUSTER;
    /* Never replace a shared cluster while a process is still waiting
     * for this bank; the old flags could satisfy a delayed wait. */
    for (unsigned i=0;i<PORT_POSTEF_PROCS;i++) {
        struct port_postef_task *t=&s->tasks[i];
        if (t->pcb && t->wait_bank==bank &&
            t->pcb->state==PORT_PCB_WAIT_EF) return PORT_POSTEF_INVALID;
    }
    s->common[bank-2]=events;
    return 0;
}
int port_postef_attach_common(struct port_postef_service *s,uint32_t pid,unsigned bank) {
    if (!s || bank<2 || bank>3) return PORT_POSTEF_BAD_CLUSTER;
    struct port_postef_task *t=lookup(s,pid);
    if (!t) return PORT_POSTEF_BAD_PID;
    if (!s->common[bank-2]) return PORT_POSTEF_UNASSIGNED;
    t->attached_common|=(uint8_t)(1u<<(bank-2));
    return 0;
}
static struct port_event_cluster *choose_bank(struct port_postef_service *s,
                  struct port_postef_task *t,unsigned bank) {
    if (!s || !t || bank>3) return 0;
    if (bank<2) return &t->private_banks[bank];
    if (!(t->attached_common&(1u<<(bank-2)))) return 0;
    return s->common[bank-2];
}
/* Private flags affect one process; common flags wake all registered and
 * attached processes whose wait condition has now become true. */
static void wake_satisfied(struct port_postef_service *s,unsigned bank,
                           struct port_postef_task *origin,
                           struct port_event_cluster *ce,unsigned priority_class,
                           unsigned *woken) {
    for (unsigned i=0;i<PORT_POSTEF_PROCS;i++) {
        struct port_postef_task *t=&s->tasks[i];
        if (!t->pcb || t->wait_bank!=bank ||
            t->pcb->state!=PORT_PCB_WAIT_EF ||
            (bank<2 && t!=origin) ||
            (bank>=2 && !(t->attached_common&(1u<<(bank-2))))) continue;
        if (port_event_satisfied(ce,t->pcb->wait_mask,t->pcb->wait_all)) {
            if (!port_pcb_wake_ef(t->pcb)) {
                t->wait_bank=0xff;
                t->last_boost_class=priority_class;
                ++t->wakeups;
                ++s->wakes;
                ++*woken;
            }
        }
    }
}
int port_postef_post_mask(struct port_postef_service *s,uint32_t pid,unsigned bank,
                          uint32_t mask,unsigned priority_class,unsigned *woken) {
    if (!s || !woken || !mask || priority_class>7) return PORT_POSTEF_INVALID;
    *woken=0;
    if (bank>3) return PORT_POSTEF_BAD_CLUSTER;
    struct port_postef_task *t=lookup(s,pid);
    if (!t) return PORT_POSTEF_BAD_PID;
    struct port_event_cluster *ce=choose_bank(s,t,bank);
    if (!ce) return PORT_POSTEF_UNASSIGNED;
    uint32_t added=mask&~ce->flags;
    port_event_set(ce,mask);
    ++s->posts;
    if (added) wake_satisfied(s,bank,t,ce,priority_class,woken);
    if (bank>=2 && added) {
        if (s->used==PORT_POSTEF_SYNC_SLOTS) {
            ++s->sync_failures;
            return PORT_POSTEF_SYNC_BACKLOG; /* local effect committed */
        }
        unsigned idx=(s->head+s->used)%PORT_POSTEF_SYNC_SLOTS;
        s->fifo[idx].bank=(uint8_t)bank;
        s->fifo[idx].flags=ce->flags;
        s->fifo[idx].sequence=++s->seq;
        ++s->used;
        ++s->notifications;
    }
    return added ? PORT_POSTEF_WAS_CLEAR : PORT_POSTEF_WAS_SET;
}
int port_postef_post(struct port_postef_service *s,uint32_t pid,unsigned ef_number,
                     unsigned priority_class,unsigned *woken) {
    if (ef_number>127) return PORT_POSTEF_BAD_CLUSTER;
    return port_postef_post_mask(s,pid,ef_number>>5,
                                (uint32_t)1u<<(ef_number&31u),priority_class,woken);
}
int port_postef_clear_mask(struct port_postef_service *s,uint32_t pid,
                           unsigned bank,uint32_t mask) {
    if (!s || !mask) return PORT_POSTEF_INVALID;
    if (bank>3) return PORT_POSTEF_BAD_CLUSTER;
    struct port_postef_task *t=lookup(s,pid);
    if (!t) return PORT_POSTEF_BAD_PID;
    struct port_event_cluster *ce=choose_bank(s,t,bank);
    if (!ce) return PORT_POSTEF_UNASSIGNED;
    port_event_clear(ce,mask);
    return 0;
}
int port_postef_wait(struct port_postef_service *s,uint32_t pid,unsigned bank,
                     uint32_t mask,int all) {
    if (!s || !mask) return PORT_POSTEF_INVALID;
    if (bank>3) return PORT_POSTEF_BAD_CLUSTER;
    struct port_postef_task *t=lookup(s,pid);
    if (!t) return PORT_POSTEF_BAD_PID;
    struct port_event_cluster *ce=choose_bank(s,t,bank);
    if (!ce) return PORT_POSTEF_UNASSIGNED;
    if (t->pcb->state!=PORT_PCB_RUNNING || t->wait_bank!=0xff)
        return PORT_POSTEF_INVALID;
    if (port_event_satisfied(ce,mask,all)) return PORT_POSTEF_WAIT_IMMEDIATE;
    if (port_pcb_block_ef(t->pcb,mask,all)) return PORT_POSTEF_INVALID;
    t->wait_bank=(uint8_t)bank;
    return PORT_POSTEF_WAIT_BLOCKED;
}
int port_postef_drain_one(struct port_postef_service *s) {
    if (!s || !s->used) return 0;
    struct port_postef_message m=s->fifo[s->head];
    s->head=(uint8_t)((s->head+1u)%PORT_POSTEF_SYNC_SLOTS);
    --s->used;
    if (m.bank>=2 && m.bank<=3) {
        /* A late snapshot must never roll back a bit subsequently set.
         * Replica is a demonstration of the shared common-EF update,
         * not a CPU-to-CPU transport implementation. */
        s->replica[m.bank-2].flags|=m.flags;
        ++s->delivered;
    }
    return 1;
}
unsigned port_postef_pending(const struct port_postef_service *s) {
    return s ? s->used : 0;
}
