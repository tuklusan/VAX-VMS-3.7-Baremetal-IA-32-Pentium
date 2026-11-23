#include <stdio.h>
#include <string.h>
#include "vms_dcl_exit.h"
#define CHECK(x) do{if(!(x)){fprintf(stderr,"DCL EXIT/WAIT test line %d\n",__LINE__);return 1;}}while(0)
struct fixture {int unstack,rundown,abort,cancel,timer,wait;unsigned efn;uint64_t ticks;};
static uint32_t calc(void *v,const char *expr,uint32_t *value){(void)v;
 if(!strcmp(expr,"7+2")){*value=9;return 1;}
 return 0x2u;
}
static void unstack(void*v){((struct fixture*)v)->unstack++;}
static void rundown(void*v){((struct fixture*)v)->rundown++;}
static void abort_now(void*v){((struct fixture*)v)->abort++;}
static int cancel(void*v){((struct fixture*)v)->cancel++;return 1;}
static int set(void*v,unsigned efn,uint64_t ticks){struct fixture*f=v;f->timer++;f->efn=efn;f->ticks=ticks;return 1;}
static int wait(void*v,unsigned efn){struct fixture*f=v;f->wait++;return f->efn==efn;}
int main(void){struct fixture f={0};const struct port_dcl_ops ops={
 .binary_expression=calc,.unstack=unstack,.rundown=rundown,.abort_process=abort_now,
 .cancel_timers=cancel,.set_timer=set,.wait_flag=wait};
 struct port_dcl_process p={.last_status=1u,.indirect_depth=2u,.interactive=1u};uint32_t status=0;
 CHECK(port_dcl_exit(&p,"7+2",&ops,&f,&status)==PORT_DCL_EXIT_RETURN);
 CHECK(p.last_status==9u&&status==9u&&p.indirect_depth==1u&&f.unstack==1&&p.command_flags==1u);
 CHECK(port_dcl_exit(&p,"",&ops,&f,&status)==PORT_DCL_EXIT_RETURN);
 CHECK(p.last_status==(9u|PORT_DCL_INHIB_MSG)&&p.indirect_depth==0&&f.unstack==2);
 CHECK(port_dcl_exit(&p,"bad",&ops,&f,&status)==PORT_DCL_EXIT_PARSE_ERROR);
 CHECK(status==2u&&p.last_status==(9u|PORT_DCL_INHIB_MSG)&&f.rundown==0);
 CHECK(port_dcl_exit(&p,"",&ops,&f,&status)==PORT_DCL_EXIT_RETURN);
 CHECK(f.rundown==1&&p.command_flags==1u);
 p=(struct port_dcl_process){.last_status=1u,.interactive=0u};
 CHECK(port_dcl_exit(&p,"",&ops,&f,&status)==PORT_DCL_EXIT_LOGOUT);
 CHECK(f.abort==1&&p.exiting==1);
 CHECK(port_dcl_wait_delta("00:00:01.50",&f.ticks)&&f.ticks==15000000u);
 CHECK(port_dcl_wait_delta("23:59:59.9999999",&f.ticks)&&f.ticks==863999999999u);
 CHECK(!port_dcl_wait_delta("24:00:00",&f.ticks));
 CHECK(!port_dcl_wait_delta("01:60:00",&f.ticks));
 CHECK(!port_dcl_wait_delta("01:59:60",&f.ticks));
 CHECK(!port_dcl_wait_delta("00:00:00.12345678",&f.ticks));
 CHECK(port_dcl_wait("00:00:00.02",&ops,&f));
 CHECK(f.cancel==1&&f.timer==1&&f.wait==1&&f.efn==31u&&f.ticks==200000u);
 puts("DCL EXIT expression/depth/rundown/batch and WAIT timer PASS");return 0;
}
