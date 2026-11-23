/* Host-side adversarial regression for the native ASTDEL-like executive. */
#include "vms_astdel.h"
#include <stdio.h>
#include <string.h>
#define CHECK(e) do { if (!(e)) { fprintf(stderr,"ASTDEL host FAIL line %u: %s\n",__LINE__,#e);return 1; } } while (0)
static unsigned calls[8], reclaimed;
static void record(void *p,uint32_t n) { unsigned *x=(unsigned *)p; *x+=n; }
static void release(struct vms_ast_request *r,void *unused) { (void)unused; if (!r->state) ++reclaimed; }
static void make(struct vms_ast_request *r,uint32_t pid,unsigned mode,unsigned kind,unsigned param) {
    memset(r,0,sizeof(*r));r->pid=pid;r->mode=(uint8_t)mode;
    r->kind=(uint8_t)kind;r->parameter=param;
    r->callback=record;r->context=&calls[mode];
}
static int test_registry_and_lifecycle(void) {
    struct port_astdel_engine e;
    struct port_astdel_proc ps[PORT_ASTDEL_SLOTS+1];
    port_astdel_init(&e);
    CHECK(port_astdel_find(&e,3)==NULL);
    for(unsigned i=0;i<PORT_ASTDEL_SLOTS+1;++i)port_astdel_proc_init(&ps[i],10+i);
    for(unsigned i=0;i<PORT_ASTDEL_SLOTS;++i)
        CHECK(port_astdel_register(&e,&ps[i])==PORT_ASTDEL_OK);
    CHECK(port_astdel_register(&e,&ps[0])==PORT_ASTDEL_INVALID);
    CHECK(port_astdel_register(&e,&ps[PORT_ASTDEL_SLOTS])==PORT_ASTDEL_REGISTRY_FULL);
    CHECK(port_astdel_unregister(&e,999)==PORT_ASTDEL_NOT_FOUND);
    CHECK(port_astdel_unregister(&e,13)==PORT_ASTDEL_OK);
    CHECK(port_astdel_register(&e,&ps[PORT_ASTDEL_SLOTS])==PORT_ASTDEL_OK);
    CHECK(port_astdel_find(&e,18)==&ps[PORT_ASTDEL_SLOTS]);
    return 0;
}
static int test_queue_and_swap(void) {
    struct port_astdel_engine e;
    struct port_astdel_proc p;
    struct vms_ast_request rs[19],replacement;
    port_astdel_init(&e);port_astdel_proc_init(&p,55);
    CHECK(port_astdel_register(&e,&p)==0);
    CHECK(port_astdel_sleep(&p,0)==0);
    CHECK(port_astdel_sleep(&p,1)!=0);
    for(unsigned i=0;i<16;++i) {
        make(&rs[i],55,3,VMS_AST_NORMAL,1);
        CHECK(port_astdel_queue(&e,&rs[i],NULL,NULL)==0);
    }
    CHECK(!p.ready && p.waiting && p.ast.queued==16);
    CHECK(port_astdel_unregister(&e,55)==PORT_ASTDEL_BUSY);
    make(&rs[16],55,0,VMS_AST_SPECIAL_KERNEL,1);
    CHECK(port_astdel_queue(&e,&rs[16],NULL,NULL)==VMS_AST_QUOTA_EXCEEDED);
    CHECK(e.queue_refused==1 && !rs[16].state);
    make(&replacement,55,3,VMS_AST_NORMAL,2);
    CHECK(port_astdel_swap(&p,&rs[5],&replacement)==PORT_ASTDEL_OK);
    CHECK(replacement.state==1 && rs[5].state==0 && p.ast.queued==16);
    CHECK(port_astdel_swap(&p,&rs[3],&replacement)==PORT_ASTDEL_INVALID);
    make(&rs[17],55,1,VMS_AST_NORMAL,1);
    CHECK(port_astdel_swap(&p,&rs[3],&rs[17])==PORT_ASTDEL_INVALID);
    CHECK(port_astdel_remove(&p,&replacement)==0);
    CHECK(port_astdel_remove(&p,&replacement)!=0);
    CHECK(p.ast.queued==15);
    for(unsigned i=0;i<16;++i) if(i!=5) CHECK(port_astdel_remove(&p,&rs[i])==0);
    CHECK(!p.ast.queued && !p.ast.quota_used);
    CHECK(port_astdel_unregister(&e,55)==0);
    return 0;
}
static int test_delivery_and_vectors(void) {
    struct port_astdel_engine e;
    struct port_astdel_proc p;
    struct vms_ast_request r[4],unknown;
    struct port_astdel_ipast ip;
    unsigned n=0;
    port_astdel_init(&e);port_astdel_proc_init(&p,71);
    CHECK(port_astdel_register(&e,&p)==0);
    make(&r[0],71,3,VMS_AST_NORMAL,1);
    make(&r[1],71,0,VMS_AST_SPECIAL_KERNEL,1);
    make(&r[2],71,3,VMS_AST_NORMAL,2);
    make(&r[3],71,1,VMS_AST_NORMAL,2);
    CHECK(port_astdel_deliver(&p,3,1,&n)==0 && n==0 && p.spurious==1);
    CHECK(port_astdel_queue(&e,&r[0],NULL,NULL)==0);
    CHECK(port_astdel_queue(&e,&r[2],NULL,NULL)==0);
    CHECK(port_astdel_queue(&e,&r[3],NULL,NULL)==0);
    CHECK(port_astdel_queue(&e,&r[1],NULL,NULL)==0);
    CHECK(port_astdel_new_level(&p)==0);
    CHECK(vms_ast_set_ipl(&p.ast,3)==0);
    CHECK(port_astdel_deliver(&p,3,100,&n)==0 && n==0);
    CHECK(vms_ast_set_ipl(&p.ast,0)==0);
    CHECK(port_astdel_deliver(&p,0,100,&n)==0 && n==1 && calls[0]==1);
    CHECK(port_astdel_deliver(&p,3,100,&n)==0 && n==3);
    CHECK(calls[1]==2 && calls[3]==3);
    CHECK(port_astdel_new_level(&p)==4);
    CHECK(port_astdel_set_vector(&e,0,record,&calls[0])==0);
    CHECK(port_astdel_set_vector(&e,PORT_ASTDEL_VECTORS,record,&calls[0])==PORT_ASTDEL_INVALID);
    memset(&ip,0,sizeof(ip));
    CHECK(port_astdel_queue_ipast(&e,71,&ip,0,4)==0);
    CHECK(port_astdel_queue_ipast(&e,71,&ip,0,4)==PORT_ASTDEL_INVALID);
    CHECK(port_astdel_deliver(&p,3,5,&n)==0 && n==1);
    CHECK(e.vector_calls==1 && calls[0]==5);
    CHECK(port_astdel_queue_ipast(&e,71,&ip,3,4)==0);
    CHECK(port_astdel_deliver(&p,3,5,&n)==0 && n==1);
    CHECK(e.vector_rejected==1);
    make(&unknown,83,3,VMS_AST_NORMAL,1);
    CHECK(port_astdel_queue(&e,&unknown,release,NULL)==PORT_ASTDEL_NOT_FOUND);
    CHECK(reclaimed==1 && !unknown.state && e.dropped_unknown_pid==1);
    CHECK(port_astdel_unregister(&e,71)==0);
    return 0;
}
static int test_resident_and_budget(void) {
    struct port_astdel_engine e;
    struct port_astdel_proc p;
    struct vms_ast_request r[3];
    unsigned n=0;
    port_astdel_init(&e);
    port_astdel_proc_init(&p,111);
    CHECK(port_astdel_register(&e,&p)==0);
    CHECK(port_astdel_sleep(&p,1)==0);
    CHECK(port_astdel_resident(&p,0)==0);
    for (unsigned i=0;i<3;i++) {
        make(&r[i],111,3,VMS_AST_NORMAL,1);
        CHECK(port_astdel_queue(&e,&r[i],NULL,NULL)==0);
    }
    CHECK(p.wakeups==1 && !p.ready);
    CHECK(port_astdel_new_level(&p)==4);
    CHECK(port_astdel_deliver(&p,3,10,&n)==PORT_ASTDEL_WOULD_BLOCK && n==0);
    CHECK(port_astdel_resident(&p,1)==0 && p.ready);
    CHECK(port_astdel_new_level(&p)==3);
    CHECK(port_astdel_deliver(&p,3,1,&n)==0 && n==1 && p.ast.queued==2);
    CHECK(port_astdel_deliver(&p,3,1,&n)==0 && n==1 && p.ast.queued==1);
    CHECK(port_astdel_deliver(&p,3,1,&n)==0 && n==1 && p.ast.queued==0);
    CHECK(port_astdel_unregister(&e,111)==0);
    return 0;
}
static int test_deferred_and_active(void) {
    struct port_astdel_engine e;
    struct port_astdel_proc p;
    struct vms_ast_request first,second,*got=0;
    unsigned n=123;
    port_astdel_init(&e);port_astdel_proc_init(&p,99);
    CHECK(port_astdel_register(&e,&p)==0);
    make(&first,99,3,VMS_AST_NORMAL,1);
    make(&second,99,3,VMS_AST_NORMAL,1);
    CHECK(port_astdel_queue(&e,&first,NULL,NULL)==0);
    CHECK(port_astdel_queue(&e,&second,NULL,NULL)==0);
    CHECK(port_astdel_set_enable(&p,3,0)==1);
    CHECK(port_astdel_new_level(&p)==4);
    CHECK(port_astdel_deliver(&p,3,5,&n)==0 && n==0);
    CHECK(port_astdel_set_enable(&p,3,1)==0);
    CHECK(vms_ast_take(&p.ast,3,&got)==0 && got==&first);
    CHECK(port_astdel_new_level(&p)==4);
    CHECK(port_astdel_deliver(&p,3,5,&n)==0 && n==0);
    CHECK(port_astdel_unregister(&e,99)==PORT_ASTDEL_BUSY);
    CHECK(vms_ast_complete(&p.ast,&first)==0);
    CHECK(port_astdel_deliver(&p,3,5,&n)==0 && n==1);
    CHECK(port_astdel_unregister(&e,99)==0);
    return 0;
}
int main(void) {
    CHECK(port_astdel_selftest()==0);
    CHECK(test_registry_and_lifecycle()==0);
    CHECK(test_queue_and_swap()==0);
    CHECK(test_delivery_and_vectors()==0);
    CHECK(test_deferred_and_active()==0);
    CHECK(test_resident_and_budget()==0);
    puts("ASTDEL host PASS: PID route/dispose, quota, sorted delivery, KAST, IPAST, wake/swap/remove, active/IPL, spurious");
    return 0;
}
