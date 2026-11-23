#include "vms_condition.h"
#include <stdio.h>
#include <string.h>
#define CHECK(x) do {if (!(x)) {fprintf(stderr,"CONDITION FAIL at line %d: %s\n",__LINE__,#x);return 1;}} while (0)
static unsigned top_calls,bottom_calls;
static enum port_condition_action resignal(const struct port_condition *c,void *arg) {
    (void)c;(void)arg; ++top_calls; return PORT_COND_RESIGNAL;
}
static enum port_condition_action continue_it(const struct port_condition *c,void *arg) {
    (void)c;(void)arg; ++bottom_calls; return PORT_COND_CONTINUE;
}
static enum port_condition_action unwind_it(const struct port_condition *c,void *arg) {
    (void)c;(void)arg; return PORT_COND_UNWIND;
}
static enum port_condition_action recurse(const struct port_condition *c,void *arg) {
    struct port_condition_process *p=arg;
    (void)port_condition_raise(p,c);
    return PORT_COND_RESIGNAL;
}
int main(void) {
    struct port_condition_process p;
    struct port_condition_handler lower={0},upper={0},other={0},rec={0};
    struct port_condition c={0};
    port_condition_init(&p);
    CHECK(port_condition_from_x86(0)==PORT_COND_DIVIDE);
    CHECK(port_condition_from_x86(3)==PORT_COND_BREAK);
    CHECK(port_condition_from_x86(6)==PORT_COND_BAD_OPCODE);
    CHECK(port_condition_from_x86(14)==PORT_COND_PAGE);
    CHECK(port_condition_from_x86(13)==PORT_COND_PROTECTION);
    CHECK(port_condition_from_x86(18)==PORT_COND_MACHINE);
    CHECK(port_condition_from_x86(255)==PORT_COND_UNKNOWN);
    CHECK(strcmp(port_condition_name(PORT_COND_AST_STACK),"AST stack fault")==0);
    c.mode=0; c.vector=3; c.type=PORT_COND_BREAK; c.continuable=1;
    c.eip=0x12345678;
    CHECK(port_condition_raise(&p,&c)==PORT_COND_STOP);
    CHECK(p.fatal==1 && p.last.eip==0x12345678);
    CHECK(port_condition_push(&p,&lower,0,continue_it,0)==0);
    CHECK(port_condition_push(&p,&upper,0,resignal,0)==0);
    CHECK(port_condition_push(&p,&upper,0,resignal,0)==-1);
    CHECK(port_condition_raise(&p,&c)==PORT_COND_CONTINUE);
    CHECK(top_calls==1 && bottom_calls==1 && p.handled==1);
    CHECK(port_condition_pop(&p,&lower)==-1); /* LIFO enforced */
    c.continuable=0;
    CHECK(port_condition_raise(&p,&c)==PORT_COND_STOP);
    CHECK(p.fatal==2); /* no unsafe resumption */
    CHECK(port_condition_pop(&p,&upper)==0);
    CHECK(port_condition_pop(&p,&lower)==0);
    CHECK(port_condition_pop(&p,&lower)==-1);
    CHECK(port_condition_push(&p,&other,3,unwind_it,0)==0);
    c.mode=3;
    CHECK(port_condition_raise(&p,&c)==PORT_COND_UNWIND);
    CHECK(p.unwind_requests==1);
    c.mode=0;
    CHECK(port_condition_raise(&p,&c)==PORT_COND_STOP); /* mode isolation */
    CHECK(port_condition_pop(&p,&other)==0);
    CHECK(port_condition_push(&p,&rec,0,recurse,&p)==0);
    CHECK(port_condition_raise(&p,&c)==PORT_COND_STOP);
    CHECK(p.signal_depth==0); /* nested signals cannot leave stale depth */
    CHECK(p.fatal>=4);
    CHECK(port_condition_pop(&p,&rec)==0);
    puts("Condition tests PASS: signals, mode isolation, resignal, continue, unwind, nesting guard");
    return 0;
}
