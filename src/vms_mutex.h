#ifndef PORTLAB_VMS_MUTEX_H
#define PORTLAB_VMS_MUTEX_H
#include <stdint.h>
#include "vms_pcb.h"

enum { PORT_MUTEX_PROCS=8, PORT_MUTEX_PER_PROC=8, PORT_MUTEX_RESOURCES=32 };
enum port_mutex_result {
    PORT_MUTEX_ACQUIRED=0, PORT_MUTEX_BLOCKED=1, PORT_MUTEX_BUSY=2,
    PORT_MUTEX_BAD_PROCESS=-1, PORT_MUTEX_BAD_STATE=-2,
    PORT_MUTEX_BAD_RESOURCE=-3, PORT_MUTEX_LIMIT=-4,
    PORT_MUTEX_NOT_OWNER=-5
};
struct port_mutex {
    int16_t count;          /* -1 = no owners; 0 = one owner */
    uint8_t write_pending;  /* blocks readers while writer waits */
};
struct port_mutex_hold {
    struct port_mutex *mutex;
    uint16_t count;
    uint8_t exclusive;
};
struct port_mutex_process {
    struct port_pcb *pcb;
    struct port_mutex_hold holds[PORT_MUTEX_PER_PROC];
    uint16_t total_held;
    uint8_t current_priority, base_priority;
    uint8_t saved_priority, saved_base;
    uint8_t waiting;
    struct port_mutex *wait_mutex;
    uint32_t wait_resource;
    uint32_t wait_order;
    uint32_t wakes;
};
struct port_mutex_service {
    struct port_mutex_process procs[PORT_MUTEX_PROCS];
    struct port_mutex io_database;
    uint32_t resource_mask, sequence, wakeups, waits, preempt_pending;
    uint8_t highest_ready_priority;
};
void port_mutex_init(struct port_mutex_service *s);
void port_mutex_init_one(struct port_mutex *m);
int port_mutex_register(struct port_mutex_service *s,struct port_pcb *p,
                        unsigned current_priority,unsigned base_priority);
int port_mutex_unregister(struct port_mutex_service *s,uint32_t pid);
int port_mutex_lockr(struct port_mutex_service *s,uint32_t pid,struct port_mutex *m);
int port_mutex_lockw(struct port_mutex_service *s,uint32_t pid,struct port_mutex *m);
int port_mutex_lockwnowait(struct port_mutex_service *s,uint32_t pid,struct port_mutex *m);
int port_mutex_unlock(struct port_mutex_service *s,uint32_t pid,struct port_mutex *m);
int port_mutex_iolockr(struct port_mutex_service *s,uint32_t pid);
int port_mutex_iolockw(struct port_mutex_service *s,uint32_t pid);
int port_mutex_iounlock(struct port_mutex_service *s,uint32_t pid);
int port_mutex_rwait(struct port_mutex_service *s,uint32_t pid,unsigned resource);
int port_mutex_ravail(struct port_mutex_service *s,unsigned resource);
uint32_t port_mutex_waiting(const struct port_mutex_service *s);
uint32_t port_mutex_selftest(void);
#endif
