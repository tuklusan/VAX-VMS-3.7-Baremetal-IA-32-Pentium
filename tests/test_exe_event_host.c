#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include "vms_exe_event.h"
static uint32_t output,posts,priority;
static int write_ok(void *unused,uint32_t address,const uint32_t *value) {
    (void)unused;
    if(address!=0x1000u)return 0;
    output=*value;return 1;
}
static uint32_t post_real(void *context,uint32_t pid,uint32_t event,uint32_t value) {
    struct port_exe_event_process *p=(struct port_exe_event_process *)context;
    assert(pid==p->pid);posts++;priority=value;
    return port_exe_event_post(p,event);
}
int main(void) {
    struct port_exe_event_process p;
    port_exe_event_init(&p,0x1122u);
    assert(port_exe_clref(&p,0u)==PORT_SS_WASCLR);
    assert(port_exe_clref(&p,31u)==PORT_SS_WASCLR);
    assert(port_exe_readef(&p,0u,0x1000u,write_ok,0)==PORT_SS_WASCLR);
    assert(output==0u);
    assert(port_exe_setef(&p,31u,8u,post_real,&p)==PORT_SS_WASCLR);
    assert(posts==1u && priority==8u);
    assert(port_exe_readef(&p,31u,0x1000u,write_ok,0)==PORT_SS_WASSET);
    assert(output==0x80000000u);
    assert(port_exe_setef(&p,31u,8u,post_real,&p)==PORT_SS_WASSET);
    assert(port_exe_clref(&p,31u)==PORT_SS_WASSET);
    assert(port_exe_clref(&p,31u)==PORT_SS_WASCLR);
    assert(port_exe_event_post(&p,32u)==PORT_SS_WASCLR);
    assert(port_exe_readef(&p,32u,0x1000u,write_ok,0)==PORT_SS_WASSET);
    assert(output==1u);
    assert(port_exe_readef(&p,32u,0x9000u,write_ok,0)==PORT_SS_ACCVIO);
    assert(port_exe_readef(&p,32u,0x1000u,0,0)==PORT_SS_ACCVIO);
    assert(port_exe_clref(&p,32u)==PORT_SS_WASSET);
    for(unsigned e=128u;e<256u;e+=31u)
        assert(port_exe_clref(&p,e)==PORT_SS_ILLEFC);
    assert(port_exe_setef(&p,128u,0u,post_real,&p)==PORT_SS_ILLEFC);
    assert(port_exe_clref(&p,64u)==PORT_SS_UNASEFC);
    assert(port_exe_readef(&p,96u,0x1000u,write_ok,0)==PORT_SS_UNASEFC);
    struct port_exe_common_event master={0};
    struct port_exe_common_event slave={.master=&master,.slave=1u};
    p.common[0]=&slave;
    assert(port_exe_event_post(&p,73u)==PORT_SS_WASCLR);
    assert(master.mask==(1u<<9));
    assert(port_exe_readef(&p,73u,0x1000u,write_ok,0)==PORT_SS_WASSET);
    assert(port_exe_clref(&p,73u)==PORT_SS_WASSET && master.mask==0u);
    p.common[1]=&master;
    assert(port_exe_event_post(&p,127u)==PORT_SS_WASCLR);
    assert(master.mask==0x80000000u);
    assert(port_exe_setef(&p,0u,0u,0,0)==PORT_SS_BADQUEUEHDR);
    assert(p.local[0]==0u);
    assert(port_exe_clref(0,0u)==PORT_SS_ILLEFC);
    puts("SYSEVTSRV local/common/slave clusters and scheduler delegation PASS");
    return 0;
}
