#include <assert.h>
#include <stdio.h>
#include "vms_lgi_protcli.h"
struct rig {struct port_exe_page_state p[3];unsigned locks,unlocks,invalidates,starts,rundowns,loads;};
static uint32_t lock(void *v){((struct rig *)v)->starts++;return 1;}
static void enter(void *v){((struct rig *)v)->locks++;}
static void leave(void *v){((struct rig *)v)->unlocks++;}
static uint32_t get(void *v,uint32_t a,struct port_exe_page_state *p){struct rig *r=v;assert(a>=0x2000&&a<=0x2400);*p=r->p[(a-0x2000)/512];return 1;}
static uint32_t set(void *v,const struct port_exe_page_state *p){struct rig *r=v;r->p[(p->virtual_address-0x2000)/512]=*p;return 1;}
static int writable(void *v,uint32_t a){(void)v;return a==0x2200;}
static void invalid(void *v,uint32_t a){(void)a;((struct rig *)v)->invalidates++;}
static uint32_t rundown(void *v){((struct rig *)v)->rundowns++;return 1;}
static uint32_t act(void *v){((struct rig *)v)->loads++;return 1;}
int main(void){struct rig r={0};
 for(unsigned i=0;i<3;i++){r.p[i].virtual_address=0x2000+i*512;r.p[i].exists=1;r.p[i].owner_mode=i==1?2:3;r.p[i].protection=14;r.p[i].type=PORT_EXE_PTE_VALID;}
 r.p[1].owner_mode=3;
 struct port_lgi_protcli_ops op={&r,lock,get,set,writable,invalid,enter,leave};
 uint32_t ranges[2]={0x2406,0x2055};
 assert(port_lgi_protect_cli(ranges,&op)==1);
 assert(r.starts==1&&r.locks==1&&r.unlocks==1&&r.invalidates==3);
 assert(r.p[0].owner_mode==2&&r.p[1].owner_mode==2&&r.p[2].owner_mode==2);
 assert(r.p[1].protection==12&&r.p[0].protection==14);
 struct port_lgi_execute_ops e={&r,rundown,act};
 assert(port_lgi_execute_image(&e)==PORT_SS_BADPARAM);
 assert(r.rundowns==1&&r.loads==1);
 puts("PASS: LOGIN CLI supervisor ownership, page protections and image handoff");}
