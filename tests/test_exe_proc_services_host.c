#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include "vms_exe_astcon.h"
#include "vms_exe_wait.h"
#include "vms_exe_dclexh.h"
#include "vms_exe_exit.h"
#include "vms_exe_forcex.h"
struct state {
    uint8_t mem[512];
    unsigned recalc,queued,freed,locked,unlocked,slept,invoked,deleted;
    uint8_t mode;
    uint32_t queued_pid,queued_handler,queued_param,exit_code;
    struct port_exe_event_process *events;
    struct port_exe_force_process *target;
};
static int r(void *c,uint32_t a,void *p,uint32_t n){struct state *s=c;if(a>512||n>512-a)return 0;memcpy(p,s->mem+a,n);return 1;}
static int w(void *c,uint32_t a,const void *p,uint32_t n){struct state *s=c;if(a>512||n>512-a)return 0;memcpy(s->mem+a,p,n);return 1;}
static int cr(void *c,uint32_t a,uint32_t n){(void)c;return a<=512&&n<=512-a;}
static void ast_level(void *c,struct port_exe_astcon_process *p){(void)p;((struct state *)c)->recalc++;}
static uint32_t ast_alloc(void *c,void **b){*b=c;return 1;}
static uint32_t ast_enq(void *c,void *b,uint32_t pid,uint32_t h,uint32_t par,uint8_t mode){struct state *s=c;assert(b==c);s->queued++;s->mode=mode;s->queued_pid=pid;s->queued_handler=h;s->queued_param=par;return 1;}
static void ast_free(void *c,void *b){assert(c==b);((struct state *)c)->freed++;}
static void lock(void *c){((struct state *)c)->locked++;}
static void unlock(void *c){((struct state *)c)->unlocked++;}
static uint32_t sleep_locked(void *c,struct port_exe_wait_process *p){struct state *s=c;s->slept++;assert(p->waiting);s->events->local[0]|=1u<<4;return 1;}
static uint32_t invoke(void *c,uint8_t mode,uint32_t handler,uint32_t arg){struct state *s=c;assert(mode==3);assert(handler==0x1234);assert(arg==0x55);s->invoked++;return 1;}
static uint32_t delete_self(void *c,uint32_t pid,uint32_t code){struct state *s=c;assert(pid==7);s->deleted++;s->exit_code=code;return 1;}
static uint32_t resolve(void *c,uint32_t p,uint32_t n,uint32_t cur,struct port_exe_force_process **target){struct state *s=c;(void)p;(void)n;(void)cur;*target=s->target;return 1;}
static uint32_t queue_exit(void *c,uint32_t pid,uint32_t code){struct state *s=c;s->queued++;s->queued_pid=pid;s->exit_code=code;return 1;}
static uint32_t deliver_exit(void *c,uint32_t pid,uint32_t code){struct state *s=c;assert(pid==9);s->exit_code=code;return 1;}
static uint32_t mem_u32(struct state *s,uint32_t a){uint32_t v;assert(r(s,a,&v,4));return v;}
int main(void){
    struct state s={0};
    struct port_exe_astcon_process ast={.pid=7,.ast_available=1,.previous_mode=2,.ast_enabled=4};
    struct port_exe_astcon_ops ao={&s,ast_level,ast_alloc,ast_enq,ast_free};
    assert(port_exe_setast(&ast,0,&ao)==PORT_SS_WASSET);
    assert(ast.ast_enabled==0&&s.recalc==1);
    assert(port_exe_setast(&ast,1,&ao)==PORT_SS_NORMAL);
    assert(ast.ast_enabled==4&&s.recalc==2);
    assert(port_exe_dclast(&ast,0xaaaa,0xbbbb,0,&ao)==1);
    assert(ast.ast_available==0&&s.queued==1&&s.mode==2&&s.queued_pid==7);
    assert(port_exe_dclast(&ast,1,2,3,&ao)==PORT_SS_EXQUOTA);
    struct port_exe_event_process events;
    port_exe_event_init(&events,7);
    s.events=&events;
    struct port_exe_wait_process waitp={.events=&events,.pid=7,.pending_ast_mode=4};
    struct port_exe_wait_ops wo={&s,lock,unlock,sleep_locked,NULL,NULL};
    assert(port_exe_waitfr(&waitp,4,&wo)==1);
    assert(s.slept==1&&s.locked==2&&s.unlocked==2);
    assert(port_exe_wfland(&waitp,0,1u<<4,&wo)==1);
    assert(port_exe_wflor(&waitp,129,1,&wo)==PORT_SS_ILLEFC);
    assert(port_exe_wflor(&waitp,64,1,&wo)==PORT_SS_UNASEFC);
    struct port_exe_access mem={&s,r,w,cr,cr,0};
    struct port_exe_exit_handlers lists={0};
    struct port_exe_dclexh_ops dco={&mem,&s,NULL};
    uint32_t first[4]={0,0x1234,0x55,400};
    uint32_t second[4]={0,0x5678,0x66,404};
    assert(w(&s,64,first,sizeof(first)));
    assert(w(&s,96,second,sizeof(second)));
    assert(port_exe_dclexh(&lists,3,64,&dco)==1);
    assert(port_exe_dclexh(&lists,3,96,&dco)==1);
    assert(lists.head[3]==96&&mem_u32(&s,96)==64);
    assert(port_exe_canexh(&lists,3,96,&dco)==1);
    assert(lists.head[3]==64);
    assert(port_exe_canexh(&lists,3,96,&dco)==PORT_SS_NOHANDLER);
    assert(port_exe_dclexh(&lists,0,64,&dco)==PORT_SS_IVSSRQ);
    assert(port_exe_dclexh(&lists,3,510,&dco)==PORT_SS_ACCVIO);
    struct port_exe_exit_process exitp={.pid=7,.previous_mode=3,.force_exit_pending=1,.handlers=&lists};
    struct port_exe_exit_ops eo={&mem,&s,invoke,delete_self};
    assert(port_exe_exit(&exitp,0x41,&eo)==1);
    assert(s.invoked==1&&s.deleted==1&&exitp.final_status==0x41);
    assert(mem_u32(&s,64)==0&&mem_u32(&s,400)==0x41);
    assert(!lists.head[3]&&!exitp.force_exit_pending);
    struct port_exe_force_process caller={.pid=7},target={.pid=9};
    s.target=&target;
    struct port_exe_force_ops fo={&s,resolve,queue_exit,deliver_exit};
    assert(port_exe_forcex(&caller,0,0,0x48,&fo)==1);
    assert(target.force_pending&&s.queued_pid==9&&s.exit_code==0x48);
    assert(port_exe_forcex(&caller,0,0,0x49,&fo)==1&&s.queued==2);
    assert(port_exe_forcex_deliver(&target,0x48,&fo)==1&&s.exit_code==0x48);
    puts("PASS: five source-equivalent process and AST service modules");
}
