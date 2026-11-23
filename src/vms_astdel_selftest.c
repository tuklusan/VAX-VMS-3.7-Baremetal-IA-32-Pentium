#include "vms_astdel.h"
/* Single deterministic harness shared by the native i386 guest and hosts.
 * Each error bit represents a major ASTDEL.MAR responsibility.
 */
struct check_context {
    unsigned count;
    unsigned kast_at_ipl2;
    uint32_t sequence;
    struct port_astdel_proc *target;
};
static void count(void *ctx,uint32_t param) {
    struct check_context *c=(struct check_context *)ctx;
    c->sequence=(c->sequence<<4)|(param&15u);
    ++c->count;
    if (c->target && c->target->ast.software_ipl==2) ++c->kast_at_ipl2;
}
static void reclaim(struct vms_ast_request *r,void *ctx) {
    struct check_context *c=(struct check_context *)ctx;
    if (r->state==0) ++c->count;
}
static void req_init(struct vms_ast_request *r,uint32_t pid,unsigned kind,
                     unsigned mode,unsigned param,struct check_context *ctx) {
    r->next=0;r->pid=pid;r->mode=(uint8_t)mode;r->kind=(uint8_t)kind;
    r->state=0;r->parameter=param;r->context=ctx;r->callback=count;
}
uint32_t port_astdel_selftest(void) {
    struct port_astdel_engine e;
    struct port_astdel_proc p,q;
    struct port_astdel_ipast ip;
    struct vms_ast_request r[6];
    struct check_context c={0,0,0,0},d={0,0,0,0};
    unsigned n=0;
    uint32_t errors=0;
    port_astdel_init(&e);
    port_astdel_proc_init(&p,0xA501u);
    port_astdel_proc_init(&q,0xA502u);
    c.target=&p;
    if (port_astdel_register(&e,&p)!=0 ||
        port_astdel_register(&e,&q)!=0 ||
        port_astdel_register(&e,&p)==0 ||
        port_astdel_find(&e,p.ast.pid)!=&p) errors|=1u;
    req_init(&r[0],p.ast.pid,VMS_AST_NORMAL,3,1,&c);
    req_init(&r[1],p.ast.pid,VMS_AST_NORMAL,0,2,&c);
    req_init(&r[2],p.ast.pid,VMS_AST_SPECIAL_KERNEL,0,3,&c);
    req_init(&r[3],p.ast.pid,VMS_AST_NORMAL,3,4,&c);
    if (port_astdel_sleep(&p,1)!=0 || p.ready ||
        port_astdel_queue(&e,&r[0],0,0)!=0 || !p.ready || p.wakeups!=1 ||
        port_astdel_queue(&e,&r[1],0,0)!=0 ||
        port_astdel_queue(&e,&r[2],0,0)!=0 ||
        port_astdel_new_level(&p)!=0) errors|=2u;
    if (port_astdel_set_enable(&p,3,0)!=1 ||
        port_astdel_deliver(&p,3,2,&n)!=0 || n!=2 ||
        c.sequence!=0x32 || c.kast_at_ipl2!=1 ||
        port_astdel_new_level(&p)!=4) errors|=4u;
    if (port_astdel_set_enable(&p,3,1)!=0 ||
        port_astdel_queue(&e,&r[3],0,0)!=0 ||
        port_astdel_deliver(&p,2,3,&n)!=0 || n!=0 ||
        port_astdel_deliver(&p,3,2,&n)!=0 || n!=2 ||
        c.sequence!=0x3214) errors|=8u;
    req_init(&r[0],p.ast.pid,VMS_AST_NORMAL,3,5,&c);
    req_init(&r[1],p.ast.pid,VMS_AST_NORMAL,3,6,&c);
    req_init(&r[4],p.ast.pid,VMS_AST_NORMAL,3,7,&c);
    if (port_astdel_queue(&e,&r[0],0,0)!=0 ||
        port_astdel_queue(&e,&r[1],0,0)!=0 ||
        port_astdel_swap(&p,&r[0],&r[4])!=0 ||
        r[0].state || r[4].state!=1 ||
        port_astdel_remove(&p,&r[1])!=0 ||
        port_astdel_remove(&p,&r[1])==0 ||
        port_astdel_deliver(&p,3,2,&n)!=0 || n!=1 ||
        (c.sequence&15u)!=7 || p.ast.queued) errors|=16u;
    req_init(&r[5],0xFFF123u,VMS_AST_NORMAL,3,8,&d);
    if (port_astdel_queue(&e,&r[5],reclaim,&d)!=PORT_ASTDEL_NOT_FOUND ||
        e.dropped_unknown_pid!=1 || d.count!=1 || r[5].state) errors|=32u;
    ip.acb.state=0;
    if (port_astdel_set_vector(&e,4,count,&c)!=0 ||
        port_astdel_queue_ipast(&e,p.ast.pid,&ip,4,9)!=0 ||
        port_astdel_deliver(&p,3,2,&n)!=0 || n!=1 ||
        e.vector_calls!=1 || c.kast_at_ipl2!=2 || (c.sequence&15u)!=9)
        errors|=64u;
    if (port_astdel_unregister(&e,p.ast.pid)!=PORT_ASTDEL_OK ||
        port_astdel_unregister(&e,p.ast.pid)!=PORT_ASTDEL_NOT_FOUND ||
        port_astdel_unregister(&e,q.ast.pid)!=PORT_ASTDEL_OK)
        errors|=128u;
    return errors;
}
