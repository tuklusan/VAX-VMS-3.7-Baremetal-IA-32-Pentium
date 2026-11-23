#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include "vms_dcl_if.h"
#define CHECK(x) do{if(!(x)){fprintf(stderr,"DCL IF line %d\n",__LINE__);return 1;}}while(0)
struct fixture {unsigned eval,marks,starts;const char *command;};
static uint32_t expr(void *p,const char *e,uint32_t*v){struct fixture*f=p;f->eval++;
 if(!strcmp(e,"bad"))return 2u;
 if(!strcmp(e,"1")){*v=1u;return 1u;}
 if(!strcmp(e,"0")){*v=0u;return 1u;}
 return 2u;
}
static void mark(void *p){((struct fixture*)p)->marks++;}
static int start(void *p,const char *s){struct fixture*f=p;f->starts++;f->command=s;return 1;}
int main(void){struct fixture f={0};struct port_dcl_if_ops ops={.normal_status=1u,.invalid_then_status=0x12u,
 .binary_expression=expr,.mark=mark,.command_start=start};
 uint32_t flags=0,rc=0;
 CHECK(port_dcl_if("1","THEN","SHOW TIME",&flags,&ops,&f,&rc)==2);
 CHECK(flags==4u&&rc==1u&&f.starts==1u&&!strcmp(f.command,"SHOW TIME"));
 CHECK(port_dcl_if("0","THEN","EXIT",&flags,&ops,&f,&rc)==1);
 CHECK(f.starts==1u&&rc==1u);
 CHECK(port_dcl_if("bad","THEN","EXIT",&flags,&ops,&f,&rc)==0);
 CHECK(rc==2u&&f.starts==1u&&f.marks==0u);
 CHECK(port_dcl_if("1","WHEN","EXIT",&flags,&ops,&f,&rc)==0);
 CHECK(rc==0x12u&&f.marks==1u&&f.starts==1u);
 CHECK(port_dcl_if("1","THE","EXIT",&flags,&ops,&f,&rc)==0);
 CHECK(f.marks==2u&&f.starts==1u);
 puts("DCL IF expression, THEN syntax, flags, branch and error PASS");return 0;
}
