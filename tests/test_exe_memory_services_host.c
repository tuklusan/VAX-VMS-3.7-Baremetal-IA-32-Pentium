#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include "vms_exe_setprt.h"
#include "vms_exe_adjwsl.h"
#include "vms_exe_purgws.h"
struct rig {uint8_t mem[512];struct port_exe_page_state pages[3];unsigned invalidated,changed,freed,expanded,lock,unlock;};
static int read_user(void *p,uint32_t a,void *out,uint32_t n){struct rig *r=p;if(a>512||n>512-a)return 0;memcpy(out,r->mem+a,n);return 1;}
static int write_user(void *p,uint32_t a,const void *in,uint32_t n){struct rig *r=p;if(a>512||n>512-a)return 0;memcpy(r->mem+a,in,n);return 1;}
static int probe(void *p,uint32_t a,uint32_t n){(void)p;return a<=512&&n<=512-a;}
static uint32_t get_page(void *p,uint32_t va,struct port_exe_page_state *out){struct rig *r=p;if(va>=1536||va%512)return PORT_SS_LENVIO;*out=r->pages[va/512];return 1;}
static uint32_t set_page(void *p,const struct port_exe_page_state *src){struct rig *r=p;assert(src->virtual_address<1536);r->pages[src->virtual_address/512]=*src;r->changed++;return 1;}
static uint32_t fault_page(void *p,uint32_t va){struct rig *r=p;r->pages[va/512].type=PORT_EXE_PTE_PAGEFILE;return 1;}
static uint32_t dec_section(void *p,uint32_t pfn){(void)p;assert(pfn==7);return 1;}
static void invalidate(void *p,uint32_t addr){struct rig *r=p;assert(addr<1536);r->invalidated++;}
static void lock(void *p){((struct rig *)p)->lock++;}
static void unlock(void *p){((struct rig *)p)->unlock++;}
static uint32_t free_wsle(void *p,struct port_exe_ws_entry *e){(void)e;((struct rig *)p)->freed++;return 1;}
static uint32_t unlock_global(void *p,struct port_exe_ws_entry *e){(void)p;e->ws_locked=0;return 1;}
static uint32_t expand(void *p,uint32_t need,uint32_t *actual){struct rig *r=p;r->expanded++;*actual=need;return 1;}
static uint32_t freepage(void *p){((struct rig *)p)->freed++;return 1;}
static uint32_t wait_resource(void *p,uint32_t cause){(void)p;(void)cause;return 1;}
int main(void){
 struct rig rig={0};
 struct port_exe_access mem={&rig,read_user,write_user,probe,probe};
 struct port_exe_setprt_ops op={&rig,&mem,get_page,set_page,fault_page,dec_section,invalidate,lock,unlock};
 int32_t quota=3;
 struct port_exe_setprt_request rq={.input_range=100,.return_range=112,.old_protection=124,.requested_mode=0,.protection=6};
 uint32_t ranges[2]={0,1023};
 assert(write_user(&rig,100,ranges,8));
 rig.pages[0]=(struct port_exe_page_state){.exists=1,.owner_mode=3,.protection=7,.type=PORT_EXE_PTE_PAGEFILE,.virtual_address=0};
 rig.pages[1]=(struct port_exe_page_state){.exists=1,.owner_mode=3,.protection=7,.type=PORT_EXE_PTE_SECTION,.virtual_address=512,.section_writable=0};
 assert(port_exe_setprt(&rq,3,&quota,&op)==1);
 assert(quota==2&&rig.changed==2&&rig.invalidated==2);
 assert(rig.pages[0].protection==6&&rig.pages[1].copy_on_reference);
 assert(rig.mem[124]==7);
 rq.protection=1;
 assert(port_exe_setprt(&rq,3,&quota,&op)==PORT_SS_IVPROTECT);
 rq.protection=4;
 rig.pages[1].type=PORT_EXE_PTE_GLOBAL;rig.pages[1].global_crf=0;
 assert(port_exe_setprt(&rq,3,&quota,&op)==PORT_SS_NOPRIV);
 struct port_exe_ws_entry entries[4]={
    {.virtual_address=0,.valid=1,.owner_mode=3},
    {.virtual_address=512,.valid=1,.owner_mode=3},
    {.virtual_address=1024,.valid=1,.owner_mode=3,.ws_locked=1,.global_page=1},
    {.virtual_address=1536,.valid=1,.owner_mode=3}
 };
 struct port_exe_purgws_ops pop={.memory=&mem,.context=&rig,.entries=entries,.entry_count=4,.locked_count=0,.lock_ws=lock,.unlock_ws=unlock,.unlock_global=unlock_global,.free_entry=free_wsle};
 ranges[0]=0;ranges[1]=2047;
 assert(write_user(&rig,100,ranges,8));
 unsigned n=0;
 assert(port_exe_purgws(100,3,&pop,&n)==1&&n==3);
 assert(entries[2].valid);
 assert(port_exe_purgws_scan(1024,1535,3,1,&pop,&n)==1 && n==1);
 assert(!entries[2].valid);
 assert(port_exe_purgws(510,3,&pop,&n)==PORT_SS_ACCVIO);
 struct port_exe_working_set ws={.size=10,.first_index=0,.first_dynamic=2,
 .last_index=11,.extent=50,.min_ws=1,.fluid=1,.pt_max=5,.pt_locked=2,
 .used_process=5,.used_global=1,.peak=9,.image_peak=8,.image_counting=1,
 .physical_pages=100,.free_limit=8,.system_max_ws=80};
 struct port_exe_adjwsl_ops wop={.memory=&mem,.context=&rig,.expand_header=expand,.free_page=freepage,.wait_resource=wait_resource};
 assert(port_exe_adjwsl(&ws,5,140,&wop)==1);
 assert(ws.size==15&&rig.expanded==1&&ws.peak_check==1&&ws.image_peak_check==1);
 uint32_t v=0;assert(read_user(&rig,140,&v,4)&&v==15);
 assert(port_exe_adjwsl(&ws,-3,140,&wop)==1);
 assert(ws.size==12);
 assert(port_exe_adjwsl(&ws,2,510,&wop)==PORT_SS_ACCVIO);
 puts("PASS: SYSSETPRT, SYSPURGWS and SYSADJWSL page services");
}
