/* Native AST queue core inspired by the documented VAX/VMS 3.7 semantics.
 * This file is independent code and does not contain translated DEC source.
 * It does not fabricate a process context, privilege transition or VAX REI.
 */
#include "vms_ast.h"

static int rank(const struct vms_ast_request *r) {
    return r->kind == VMS_AST_SPECIAL_KERNEL ? -1 : (int)r->mode;
}
uint8_t vms_ast_compute_level(const struct vms_ast_process *p) {
    if (!p || !p->queue) return VMS_AST_NONE_LEVEL;
    return p->queue->kind == VMS_AST_SPECIAL_KERNEL ? 0 : p->queue->mode;
}
static void refresh_level(struct vms_ast_process *p) {
    p->ast_level = vms_ast_compute_level(p);
}
void vms_ast_init(struct vms_ast_process *p, uint32_t pid) {
    if (!p) return;
    p->pid=pid; p->queue=0; p->queued=0; p->quota_used=0;
    p->delivered=0; p->enable_mask=0x0f; p->active_mask=0;
    p->software_ipl=0; p->ast_level=VMS_AST_NONE_LEVEL;
}
int vms_ast_set_enable(struct vms_ast_process *p, unsigned mode, int enabled) {
    if (!p || mode > 3) return VMS_AST_BAD_ARGUMENT;
    int old=(p->enable_mask >> mode)&1;
    if (enabled) p->enable_mask |= (uint8_t)(1u<<mode);
    else p->enable_mask &= (uint8_t)~(1u<<mode);
    refresh_level(p);
    return old;
}
int vms_ast_set_ipl(struct vms_ast_process *p, unsigned ipl) {
    if (!p || ipl > 31) return VMS_AST_BAD_ARGUMENT;
    p->software_ipl=(uint8_t)ipl;
    return VMS_AST_OK;
}
int vms_ast_enqueue(struct vms_ast_process *p, struct vms_ast_request *r) {
    if (!p || !r || !r->callback || r->mode > 3 ||
        r->kind > VMS_AST_SPECIAL_KERNEL) return VMS_AST_BAD_ARGUMENT;
    if (r->pid != p->pid) return VMS_AST_BAD_PID;
    if (r->state != 0) return VMS_AST_ALREADY_QUEUED;
    if (p->quota_used == VMS_AST_QUOTA) return VMS_AST_QUOTA_EXCEEDED;
    /* Stable order: KAST first, then normal modes 0,1,2,3; FIFO per mode. */
    struct vms_ast_request **link=&p->queue;
    while (*link && rank(*link) <= rank(r)) link=&(*link)->next;
    r->next=*link;
    *link=r;
    r->state=1;
    ++p->queued;
    ++p->quota_used;
    refresh_level(p);
    return VMS_AST_OK;
}
int vms_ast_take(struct vms_ast_process *p, unsigned returning_mode,
                 struct vms_ast_request **picked) {
    if (!p || !picked || returning_mode > 3) return VMS_AST_BAD_ARGUMENT;
    *picked=0;
    if (!p->queue) return VMS_AST_EMPTY;
    if (p->software_ipl > 2) return VMS_AST_DEFERRED;
    struct vms_ast_request *r=p->queue;
    if (r->kind == VMS_AST_NORMAL) {
        /* A more privileged mode cannot run a less-privileged AST. */
        uint8_t mask=(uint8_t)(1u<<r->mode);
        if (r->mode > returning_mode || !(p->enable_mask & mask) ||
            (p->active_mask & mask)) return VMS_AST_DEFERRED;
        p->active_mask |= mask;
    }
    p->queue=r->next;
    r->next=0;
    r->state=2;
    --p->queued;
    --p->quota_used;
    refresh_level(p);
    /* While normal AST is active, its target mode is suppressed until EXIT. */
    if (r->kind == VMS_AST_NORMAL) p->ast_level=(uint8_t)(r->mode+1);
    *picked=r;
    return VMS_AST_OK;
}
int vms_ast_complete(struct vms_ast_process *p, struct vms_ast_request *r) {
    if (!p || !r || r->pid != p->pid || r->state != 2)
        return VMS_AST_BAD_ARGUMENT;
    if (r->kind == VMS_AST_NORMAL) {
        const uint8_t mask=(uint8_t)(1u<<r->mode);
        if (!(p->active_mask & mask)) return VMS_AST_BAD_ARGUMENT;
        p->active_mask &= (uint8_t)~mask;
    }
    r->state=0;
    ++p->delivered;
    refresh_level(p);
    return VMS_AST_OK;
}
int vms_ast_cancel(struct vms_ast_process *p, struct vms_ast_request *r) {
    if (!p || !r || r->pid != p->pid) return VMS_AST_BAD_ARGUMENT;
    struct vms_ast_request **link=&p->queue;
    while (*link && *link!=r) link=&(*link)->next;
    if (!*link || r->state != 1) return VMS_AST_BAD_ARGUMENT;
    *link=r->next;
    r->next=0;
    r->state=0;
    --p->queued;
    --p->quota_used;
    refresh_level(p);
    return VMS_AST_OK;
}

int vms_ast_dispatch_one(struct vms_ast_process *p, unsigned returning_mode) {
    struct vms_ast_request *r=0;
    int rc=vms_ast_take(p,returning_mode,&r);
    if (rc!=VMS_AST_OK) return rc;
    /* Special kernel ASTs use the modeled IPL-2 context. Ordinary ASTs
     * run at the caller's safe return point (not from a hardware ISR). */
    uint8_t old_ipl=p->software_ipl;
    if (r->kind==VMS_AST_SPECIAL_KERNEL) p->software_ipl=2;
    r->callback(r->context,r->parameter);
    p->software_ipl=old_ipl;
    return vms_ast_complete(p,r);
}
