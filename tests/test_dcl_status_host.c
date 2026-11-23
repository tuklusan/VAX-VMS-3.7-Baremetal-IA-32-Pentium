#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include "vms_dcl_status.h"
#define CHECK(x) do{if(!(x)){fprintf(stderr,"DCL STATUS line %d\n",__LINE__);return 1;}}while(0)
struct calls {unsigned disable,flush,reset,start;};
static void dis(void*ctx){((struct calls*)ctx)->disable++;}
static void flush(void*ctx){((struct calls*)ctx)->flush++;}
static void reset(void*ctx){((struct calls*)ctx)->reset++;}
static void start(void*ctx){((struct calls*)ctx)->start++;}
int main(void){
    struct port_dcl_status_process p={.last_status=1u,.on_level=2u};
    struct port_dcl_status_work w={.flags=PORT_DCL_STATUS_NOSTAT};
    struct calls calls={0};
    const struct port_dcl_status_ops ops={.disable_control_ast=dis,.flush=flush,
        .on_reset=reset,.command_start=start};
    CHECK(port_dcl_set_status(&p,&w,1u,&ops,&calls)==0);
    CHECK(p.last_status==1u&&calls.disable==0);
    w.flags=0;
    CHECK(port_dcl_set_status(&p,&w,1u,&ops,&calls)==0);
    CHECK(p.last_status==1u&&!strcmp(p.status_text,"%X00000001")&&p.severity==1u);
    CHECK(port_dcl_set_status(&p,&w,2u,&ops,&calls)==PORT_DCL_STATUS_ON_COMMAND);
    CHECK(!strcmp(w.command,"EXIT")&&calls.flush==1&&calls.reset==1&&calls.start==1);
    CHECK(p.flags&PORT_DCL_STATUS_ONEXIT);
    CHECK(port_dcl_set_status(&p,&w,4u,&ops,&calls)==0);
    CHECK(!(p.flags&PORT_DCL_STATUS_ONEXIT)&&p.last_status==2u);
    p.on_level=3u;
    CHECK(port_dcl_set_status(&p,&w,2u,&ops,&calls)==0);
    CHECK(calls.flush==1u&&p.last_status==2u);
    p.on_level=2u;p.on_error="GOTO HANDLE";
    CHECK(port_dcl_set_status(&p,&w,2u,&ops,&calls)==PORT_DCL_STATUS_ON_COMMAND);
    CHECK(!strcmp(w.command,"GOTO HANDLE")&&!(p.flags&PORT_DCL_STATUS_ONEXIT));
    p.flags=PORT_DCL_STATUS_YLEVEL;
    CHECK(port_dcl_set_status(&p,&w,4u,&ops,&calls)==0);
    CHECK(p.last_status==2u&&p.severity==4u&&calls.flush==2u);
    p.flags=0;p.on_level=7u;
    CHECK(port_dcl_set_status(&p,&w,0xfffffffcu,&ops,&calls)==0);
    CHECK(p.last_status==0x00038010u && !strcmp(p.status_text,"%X00038010"));
    puts("DCL STATUS severity, symbols, ON ERROR, EXIT guard, YLEVEL PASS");
    return 0;
}
