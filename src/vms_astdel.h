#ifndef PORTLAB_VMS_ASTDEL_H
#define PORTLAB_VMS_ASTDEL_H
#include <stdint.h>
#include "vms_ast.h"

/* Original, uniprocessor IA-32 AST delivery engine.  No VAX frame/REI ABI.
 * Mirrors the responsibilities (not machine instructions) of ASTDEL.MAR:
 * process lookup/queue, recalculation, normal/KAST dispatch, IPAST vector
 * resolution, queued control-block replacement/removal and wakeup.
 * Callers serialize access to process queues; never call from an IRQ.
 */
enum { PORT_ASTDEL_SLOTS=8, PORT_ASTDEL_VECTORS=16 };
enum port_astdel_status {
    PORT_ASTDEL_OK=0, PORT_ASTDEL_NOT_FOUND=-10,
    PORT_ASTDEL_REGISTRY_FULL=-11, PORT_ASTDEL_BUSY=-12,
    PORT_ASTDEL_INVALID=-13, PORT_ASTDEL_WOULD_BLOCK=-14
};
struct port_astdel_proc {
    struct vms_ast_process ast;
    uint8_t resident;
    uint8_t waiting;
    uint8_t wake_on_ast;
    uint8_t ready;
    uint32_t arrivals;
    uint32_t wakeups;
    uint32_t normal_deliveries;
    uint32_t kast_deliveries;
    uint32_t spurious;
};
struct port_astdel_vector {
    vms_ast_callback fn;
    void *context;
};
struct port_astdel_engine {
    struct port_astdel_proc *slots[PORT_ASTDEL_SLOTS];
    struct port_astdel_vector vectors[PORT_ASTDEL_VECTORS];
    uint32_t dropped_unknown_pid;
    uint32_t queue_refused;
    uint32_t vector_calls;
    uint32_t vector_rejected;
};
struct port_astdel_ipast {
    struct vms_ast_request acb;
    struct port_astdel_engine *engine;
    uint8_t index;
};
typedef void (*port_astdel_reclaim)(struct vms_ast_request *,void *);
void port_astdel_init(struct port_astdel_engine *engine);
void port_astdel_proc_init(struct port_astdel_proc *proc,uint32_t pid);
int port_astdel_register(struct port_astdel_engine *engine,struct port_astdel_proc *proc);
int port_astdel_unregister(struct port_astdel_engine *engine,uint32_t pid);
struct port_astdel_proc *port_astdel_find(const struct port_astdel_engine *engine,uint32_t pid);
int port_astdel_sleep(struct port_astdel_proc *proc,int wake_on_ast);
int port_astdel_resident(struct port_astdel_proc *proc,int resident);
int port_astdel_queue(struct port_astdel_engine *engine,struct vms_ast_request *req,
                      port_astdel_reclaim discarded,void *discard_context);
int port_astdel_remove(struct port_astdel_proc *proc,struct vms_ast_request *req);
int port_astdel_swap(struct port_astdel_proc *proc,struct vms_ast_request *old_req,
                     struct vms_ast_request *replacement);
uint8_t port_astdel_new_level(const struct port_astdel_proc *proc);
int port_astdel_set_enable(struct port_astdel_proc *proc,unsigned mode,int enabled);
int port_astdel_deliver(struct port_astdel_proc *proc,unsigned returning_mode,
                         unsigned budget,unsigned *delivered);
int port_astdel_set_vector(struct port_astdel_engine *engine,unsigned index,
                           vms_ast_callback fn,void *context);
int port_astdel_queue_ipast(struct port_astdel_engine *engine,uint32_t pid,
                            struct port_astdel_ipast *wrapper,unsigned index,
                            uint32_t parameter);
/* Portable deterministic integration probe, also built into real x86 guest. */
uint32_t port_astdel_selftest(void);
#endif
