#include "vms_ast.h"
#include <stdio.h>
#include <string.h>
#define CHECK(x) do {if (!(x)) {fprintf(stderr,"AST FAIL at line %d: %s\n",__LINE__,#x);return 1;}} while (0)
static unsigned callbacks;
static unsigned observed_ipl;
static void check_ipl(void *p, uint32_t parameter) {
    (void)parameter; observed_ipl=((struct vms_ast_process *)p)->software_ipl;
}
static void counted(void *p, uint32_t parameter) { (void)p; callbacks+=parameter; }
static void init_req(struct vms_ast_request *r, uint32_t pid,unsigned mode,unsigned kind,unsigned parameter) {
    memset(r,0,sizeof(*r));
    r->pid=pid; r->mode=(uint8_t)mode; r->kind=(uint8_t)kind;
    r->parameter=parameter; r->callback=counted;
}
static int step(struct vms_ast_process *p,unsigned retmode,struct vms_ast_request *expected) {
    struct vms_ast_request *r=0;
    CHECK(vms_ast_take(p,retmode,&r)==VMS_AST_OK);
    CHECK(r==expected);
    CHECK(r->state==2);
    expected->callback(expected->context,expected->parameter);
    CHECK(vms_ast_complete(p,expected)==VMS_AST_OK);
    CHECK(expected->state==0);
    return 0;
}
int main(void) {
    struct vms_ast_process p,q;
    struct vms_ast_request r[18],*got;
    vms_ast_init(&p,0x100);
    vms_ast_init(&q,0x200);
    CHECK(p.ast_level==VMS_AST_NONE_LEVEL && p.enable_mask==15);
    CHECK(p.software_ipl==0 && p.active_mask==0);
    CHECK(vms_ast_set_ipl(&p,32)==VMS_AST_BAD_ARGUMENT);
    CHECK(vms_ast_set_enable(&p,4,0)==VMS_AST_BAD_ARGUMENT);
    CHECK(vms_ast_take(&p,3,&got)==VMS_AST_EMPTY && !got);
    init_req(&r[0],p.pid,3,VMS_AST_NORMAL,1);
    init_req(&r[1],p.pid,0,VMS_AST_NORMAL,2);
    init_req(&r[2],p.pid,2,VMS_AST_NORMAL,4);
    init_req(&r[3],p.pid,0,VMS_AST_SPECIAL_KERNEL,8);
    init_req(&r[4],p.pid,0,VMS_AST_SPECIAL_KERNEL,16);
    init_req(&r[5],q.pid,1,VMS_AST_NORMAL,32);
    CHECK(vms_ast_enqueue(&p,&r[0])==VMS_AST_OK);
    CHECK(p.ast_level==3);
    CHECK(vms_ast_enqueue(&p,&r[1])==VMS_AST_OK);
    CHECK(vms_ast_enqueue(&p,&r[2])==VMS_AST_OK);
    CHECK(vms_ast_enqueue(&p,&r[3])==VMS_AST_OK);
    CHECK(vms_ast_enqueue(&p,&r[4])==VMS_AST_OK);
    CHECK(vms_ast_enqueue(&p,&r[5])==VMS_AST_BAD_PID);
    CHECK(vms_ast_enqueue(&p,&r[4])==VMS_AST_ALREADY_QUEUED);
    CHECK(p.ast_level==0);
    CHECK(vms_ast_set_ipl(&p,6)==VMS_AST_OK);
    CHECK(vms_ast_take(&p,3,&got)==VMS_AST_DEFERRED && !got);
    CHECK(vms_ast_set_ipl(&p,0)==VMS_AST_OK);
    CHECK(step(&p,3,&r[3])==0);
    CHECK(step(&p,3,&r[4])==0);
    CHECK(step(&p,3,&r[1])==0);
    CHECK(vms_ast_take(&p,1,&got)==VMS_AST_DEFERRED && !got);
    CHECK(step(&p,3,&r[2])==0);
    CHECK(vms_ast_set_enable(&p,3,0)==1);
    CHECK(vms_ast_take(&p,3,&got)==VMS_AST_DEFERRED);
    CHECK(vms_ast_set_enable(&p,3,1)==0);
    CHECK(step(&p,3,&r[0])==0);
    CHECK(p.ast_level==4 && !p.queued && !p.quota_used);
    CHECK(p.delivered==5 && callbacks==31);
    /* Active mask must block reentrant delivery in the same mode. */
    init_req(&r[0],p.pid,3,VMS_AST_NORMAL,1);
    init_req(&r[1],p.pid,3,VMS_AST_NORMAL,1);
    CHECK(vms_ast_enqueue(&p,&r[0])==VMS_AST_OK);
    CHECK(vms_ast_enqueue(&p,&r[1])==VMS_AST_OK);
    CHECK(vms_ast_take(&p,3,&got)==VMS_AST_OK && got==&r[0]);
    CHECK(p.active_mask & 8);
    CHECK(vms_ast_take(&p,3,&got)==VMS_AST_DEFERRED);
    CHECK(vms_ast_complete(&p,&r[0])==VMS_AST_OK);
    CHECK(step(&p,3,&r[1])==0);
    /* Cancel only queued requests; completed and active are not cancelable. */
    init_req(&r[0],p.pid,2,VMS_AST_NORMAL,1);
    CHECK(vms_ast_enqueue(&p,&r[0])==VMS_AST_OK);
    CHECK(vms_ast_cancel(&p,&r[0])==VMS_AST_OK);
    CHECK(vms_ast_cancel(&p,&r[0])==VMS_AST_BAD_ARGUMENT);
    /* Quota and process isolation are explicit. */
    for (unsigned i=0;i<17;++i) {
        init_req(&r[i],q.pid,3,VMS_AST_NORMAL,1);
        CHECK(vms_ast_enqueue(&q,&r[i])==(i<16?VMS_AST_OK:VMS_AST_QUOTA_EXCEEDED));
    }
    CHECK(q.queued==16 && p.queued==0);
    CHECK(vms_ast_cancel(&q,&r[0])==VMS_AST_OK);
    CHECK(vms_ast_enqueue(&q,&r[16])==VMS_AST_OK);
    CHECK(vms_ast_cancel(&q,&r[16])==VMS_AST_OK);
    CHECK(q.queued==15);
    /* Dedicated callback path must run kernel ASTs at modeled IPL 2. */
    init_req(&r[17],p.pid,0,VMS_AST_SPECIAL_KERNEL,1);
    r[17].callback=check_ipl;
    r[17].context=&p;
    CHECK(vms_ast_enqueue(&p,&r[17])==VMS_AST_OK);
    CHECK(vms_ast_dispatch_one(&p,3)==VMS_AST_OK);
    CHECK(observed_ipl==2 && p.software_ipl==0);
    CHECK(p.ast_level==VMS_AST_NONE_LEVEL);
    puts("AST tests PASS: ordered KAST, FIFO modes, disable/active/IPL, quota, PID, cancel");
    return 0;
}
