/* Independent IA-32 research implementation of AST delivery and ACB lifecycle.
 * Original behavior model; no DEC source translated or included here.
 * The caller provides a serialized safe point; CPL3 dispatch in arch.c is
 * separate from this architecture-neutral process delivery policy.
 */
#include "vms_astdel.h"

void port_astdel_init(struct port_astdel_engine *e) {
    if (!e) return;
    for (unsigned i=0;i<PORT_ASTDEL_SLOTS;i++) e->slots[i]=0;
    for (unsigned i=0;i<PORT_ASTDEL_VECTORS;i++) {
        e->vectors[i].fn=0;
        e->vectors[i].context=0;
    }
    e->dropped_unknown_pid=e->queue_refused=e->vector_calls=e->vector_rejected=0;
}
void port_astdel_proc_init(struct port_astdel_proc *p,uint32_t pid) {
    if (!p) return;
    vms_ast_init(&p->ast,pid);
    p->resident=1;p->waiting=0;p->wake_on_ast=1;p->ready=1;
    p->arrivals=p->wakeups=p->normal_deliveries=p->kast_deliveries=p->spurious=0;
}
struct port_astdel_proc *port_astdel_find(const struct port_astdel_engine *e,uint32_t pid) {
    if (!e || !pid) return 0;
    for (unsigned i=0;i<PORT_ASTDEL_SLOTS;i++)
        if (e->slots[i] && e->slots[i]->ast.pid==pid) return e->slots[i];
    return 0;
}
int port_astdel_register(struct port_astdel_engine *e,struct port_astdel_proc *p) {
    if (!e || !p || !p->ast.pid || port_astdel_find(e,p->ast.pid))
        return PORT_ASTDEL_INVALID;
    for (unsigned i=0;i<PORT_ASTDEL_SLOTS;i++) {
        if (e->slots[i]==p) return PORT_ASTDEL_INVALID;
        if (!e->slots[i]) { e->slots[i]=p; return PORT_ASTDEL_OK; }
    }
    return PORT_ASTDEL_REGISTRY_FULL;
}
int port_astdel_unregister(struct port_astdel_engine *e,uint32_t pid) {
    if (!e || !pid) return PORT_ASTDEL_INVALID;
    for (unsigned i=0;i<PORT_ASTDEL_SLOTS;i++) {
        struct port_astdel_proc *p=e->slots[i];
        if (!p || p->ast.pid!=pid) continue;
        if (p->ast.queued || p->ast.active_mask || p->ast.quota_used)
            return PORT_ASTDEL_BUSY;
        e->slots[i]=0;
        return PORT_ASTDEL_OK;
    }
    return PORT_ASTDEL_NOT_FOUND;
}
int port_astdel_resident(struct port_astdel_proc *p,int resident) {
    if (!p) return PORT_ASTDEL_INVALID;
    p->resident=(uint8_t)(resident!=0);
    p->ready=(uint8_t)(p->resident && !p->waiting);
    return PORT_ASTDEL_OK;
}
int port_astdel_sleep(struct port_astdel_proc *p,int wake_on_ast) {
    if (!p || p->waiting || !p->resident) return PORT_ASTDEL_INVALID;
    p->waiting=1;
    p->wake_on_ast=(uint8_t)(wake_on_ast!=0);
    p->ready=0;
    return PORT_ASTDEL_OK;
}
/* PID lookup and disposal on no-target are distinct from a queue failure.
 * Ownership stays with the caller on invalid requests or quota errors.
 */
int port_astdel_queue(struct port_astdel_engine *e,struct vms_ast_request *r,
                      port_astdel_reclaim discard,void *ctx) {
    if (!e || !r || !r->pid || r->state || !r->callback ||
        r->mode>VMS_MODE_USER || r->kind>VMS_AST_SPECIAL_KERNEL)
        return PORT_ASTDEL_INVALID;
    struct port_astdel_proc *p=port_astdel_find(e,r->pid);
    if (!p) {
        ++e->dropped_unknown_pid;
        if (discard) discard(r,ctx);
        return PORT_ASTDEL_NOT_FOUND;
    }
    int rc=vms_ast_enqueue(&p->ast,r);
    if (rc!=VMS_AST_OK) { ++e->queue_refused; return rc; }
    ++p->arrivals;
    if (p->waiting && p->wake_on_ast) {
        p->waiting=0;
        ++p->wakeups;
        /* Nonresident targets keep queued ASTs until brought back in. */
        if (p->resident) p->ready=1;
    }
    return PORT_ASTDEL_OK;
}
int port_astdel_remove(struct port_astdel_proc *p,struct vms_ast_request *r) {
    if (!p || !r) return PORT_ASTDEL_INVALID;
    return vms_ast_cancel(&p->ast,r);
}
/* Replace in place: PID/mode/class invariant prevents silent queue reordering.
 * Failure is atomic.  Used to detach embedded ACBs before their owner frees.
 */
int port_astdel_swap(struct port_astdel_proc *p,struct vms_ast_request *old,
                     struct vms_ast_request *next) {
    if (!p || !old || !next || old==next || old->state!=1 || next->state ||
        old->pid!=p->ast.pid || next->pid!=old->pid ||
        next->mode!=old->mode || next->kind!=old->kind || !next->callback)
        return PORT_ASTDEL_INVALID;
    struct vms_ast_request **link=&p->ast.queue;
    unsigned hops=0;
    while (*link && *link!=old && hops++<VMS_AST_QUOTA) link=&(*link)->next;
    if (*link!=old) return PORT_ASTDEL_NOT_FOUND;
    next->next=old->next;
    next->state=1;
    *link=next;
    old->next=0;
    old->state=0;
    return PORT_ASTDEL_OK;
}
/* AST interrupt level for the first runnable pending mode. Disabled and
 * active modes are suppressed; special kernel ASTs remain eligible.
 */
uint8_t port_astdel_new_level(const struct port_astdel_proc *p) {
    if (!p || !p->resident || !p->ast.queue) return VMS_AST_NONE_LEVEL;
    const struct vms_ast_request *r=p->ast.queue;
    if (r->kind==VMS_AST_SPECIAL_KERNEL) return VMS_MODE_KERNEL;
    uint8_t mask=(uint8_t)(1u<<r->mode);
    return (p->ast.enable_mask&mask) && !(p->ast.active_mask&mask)
        ? r->mode : VMS_AST_NONE_LEVEL;
}
int port_astdel_set_enable(struct port_astdel_proc *p,unsigned mode,int enabled) {
    if (!p) return PORT_ASTDEL_INVALID;
    return vms_ast_set_enable(&p->ast,mode,enabled);
}
/* Bounded delivery at a scheduled return-to-mode point.  This never
 * executes callbacks from an interrupt frame, and never enters VAX ABI.
 */
int port_astdel_deliver(struct port_astdel_proc *p,unsigned mode,
                        unsigned budget,unsigned *delivered) {
    if (!p || !delivered || mode>VMS_MODE_USER || !budget)
        return PORT_ASTDEL_INVALID;
    *delivered=0;
    if (!p->resident || !p->ready) return PORT_ASTDEL_WOULD_BLOCK;
    for (unsigned i=0;i<budget;i++) {
        if (!p->ast.queue) { if (!i) ++p->spurious; break; }
        if (p->ast.software_ipl>2) break;
        struct vms_ast_request *first=p->ast.queue;
        if (first->kind==VMS_AST_NORMAL &&
            (first->mode>mode ||
             !(p->ast.enable_mask&(1u<<first->mode)) ||
             (p->ast.active_mask&(1u<<first->mode)))) break;
        unsigned special=first->kind==VMS_AST_SPECIAL_KERNEL;
        if (vms_ast_dispatch_one(&p->ast,mode)!=VMS_AST_OK)
            return PORT_ASTDEL_INVALID;
        if (special) ++p->kast_deliveries;
        else ++p->normal_deliveries;
        ++*delivered;
    }
    return PORT_ASTDEL_OK;
}
int port_astdel_set_vector(struct port_astdel_engine *e,unsigned index,
                           vms_ast_callback fn,void *context) {
    if (!e || index>=PORT_ASTDEL_VECTORS) return PORT_ASTDEL_INVALID;
    e->vectors[index].fn=fn;
    e->vectors[index].context=context;
    return PORT_ASTDEL_OK;
}
static void ipast_invoke(void *p,uint32_t param) {
    struct port_astdel_ipast *w=(struct port_astdel_ipast *)p;
    if (!w || !w->engine || w->index>=PORT_ASTDEL_VECTORS) return;
    struct port_astdel_vector *slot=&w->engine->vectors[w->index];
    if (slot->fn) {
        ++w->engine->vector_calls;
        slot->fn(slot->context,param);
    } else ++w->engine->vector_rejected;
}
int port_astdel_queue_ipast(struct port_astdel_engine *e,uint32_t pid,
                            struct port_astdel_ipast *w,unsigned index,
                            uint32_t parameter) {
    if (!e || !w || index>=PORT_ASTDEL_VECTORS || w->acb.state)
        return PORT_ASTDEL_INVALID;
    w->engine=e;
    w->index=(uint8_t)index;
    w->acb.next=0;
    w->acb.pid=pid;
    w->acb.mode=VMS_MODE_KERNEL;
    w->acb.kind=VMS_AST_SPECIAL_KERNEL;
    w->acb.callback=ipast_invoke;
    w->acb.context=w;
    w->acb.parameter=parameter;
    return port_astdel_queue(e,&w->acb,0,0);
}
