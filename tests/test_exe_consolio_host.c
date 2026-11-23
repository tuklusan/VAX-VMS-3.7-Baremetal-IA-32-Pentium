#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "vms_exe_consolio.h"
struct cstate {uint8_t out[128],in[12];unsigned outn,inn,inmax,ready_calls;uint32_t last_device;};
static int rxready(void *p){struct cstate *c=p;return c->inn<c->inmax;}
static int rx(void *p,uint8_t *v){struct cstate *c=p;if(c->inn>=c->inmax)return 0;*v=c->in[c->inn++];return 1;}
static int txready(void *p,uint32_t d){(void)d;((struct cstate *)p)->ready_calls++;return 1;}
static int tx(void *p,uint32_t d,uint8_t b){struct cstate *c=p;c->last_device=d;assert(c->outn<sizeof c->out);c->out[c->outn++]=b;return 1;}
int main(void){
    struct cstate s={0};struct port_exe_console c={&s,rxready,rx,txready,tx};
    s.in[0]=19u+128u;s.in[1]='x';s.in[2]=17u+128u;s.inmax=3;
    assert(port_exe_outhex(&c,0u,0x1234abcdu));
    assert(s.outn==8&&!memcmp(s.out,"1234ABCD",8)&&s.inn==3);
    assert(port_exe_outcrlf(&c,0u));
    assert(s.outn==12 && s.out[8]==13 && s.out[9]==0 && s.out[10]==0 && s.out[11]==10);
    uint8_t counted[]={5,'A','B',0,'X','Y'};
    assert(port_exe_outcstring(&c,1u,counted));
    assert(port_exe_outzstring(&c,1u,"xyz"));
    assert(port_exe_outblank(&c,1u));
    assert(s.last_device==1 && s.out[12]=='A' && s.out[13]=='B');
    assert(s.outn==18 && s.out[14]=='x' && s.out[15]=='y' && s.out[16]=='z' && s.out[17]==' ');
    assert(s.ready_calls==s.outn);
    assert(!port_exe_outzstring(NULL,0u,"abc"));
    puts("PASS: CONSOLIO hex, CR fill, strings, XOFF and XON");
}
