#include <assert.h>
#include <stdio.h>
#include "vms_mutex.h"
static uint32_t pd[1024] __attribute__((aligned(4096)));
static void init(struct port_pcb *p,uint32_t id) {
    assert(port_pcb_create(p,id,pd,0x24000,0x10000)==0);
    assert(port_pcb_activate(p)==0);
}
static void test_multi_owner(void) {
    struct port_mutex_service s;
    struct port_pcb p[3];
    struct port_mutex m[2];
    port_mutex_init(&s);
    port_mutex_init_one(&m[0]);port_mutex_init_one(&m[1]);
    for(unsigned i=0;i<3;i++){
        init(&p[i],i+1);
        assert(port_mutex_register(&s,&p[i],i+2,i+3)==0);
    }
    assert(port_mutex_lockr(&s,1,&m[0])==0);
    assert(port_mutex_lockr(&s,1,&m[0])==0);
    assert(port_mutex_lockr(&s,1,&m[1])==0);
    assert(s.procs[0].total_held==3 && m[0].count==1);
    assert(port_mutex_lockw(&s,2,&m[0])==PORT_MUTEX_BLOCKED);
    assert(port_mutex_unregister(&s,2)==PORT_MUTEX_BAD_STATE);
    assert(port_mutex_lockr(&s,3,&m[0])==PORT_MUTEX_BLOCKED);
    assert(port_mutex_unlock(&s,1,&m[0])==0);
    assert(m[0].count==0 && p[1].state==PORT_PCB_WAIT_RES);
    assert(port_mutex_unlock(&s,1,&m[1])==0);
    assert(s.procs[0].current_priority==15);
    assert(port_mutex_unlock(&s,1,&m[0])==0);
    assert(p[1].state==PORT_PCB_READY && p[2].state==PORT_PCB_READY);
    assert(s.procs[0].current_priority==2 && s.procs[0].base_priority==3);
    assert(port_pcb_activate(&p[1])==0);
    assert(port_mutex_lockw(&s,2,&m[0])==0);
    assert(port_pcb_activate(&p[2])==0);
    assert(port_mutex_lockwnowait(&s,3,&m[0])==PORT_MUTEX_BUSY);
    assert(port_mutex_unlock(&s,2,&m[0])==0);
    assert(port_mutex_lockr(&s,3,&m[0])==0);
    assert(port_mutex_unlock(&s,3,&m[0])==0);
    for(unsigned i=0;i<3;i++)assert(port_mutex_unregister(&s,p[i].pid)==0);
}
static void test_resources(void) {
    struct port_mutex_service s;
    struct port_pcb p[3];
    port_mutex_init(&s);
    for(unsigned i=0;i<3;i++){
        init(&p[i],i+21);
        assert(port_mutex_register(&s,&p[i],16,16)==0);
    }
    assert(port_mutex_rwait(&s,21,0)==PORT_MUTEX_BLOCKED);
    assert(port_mutex_rwait(&s,22,0)==PORT_MUTEX_BLOCKED);
    assert(port_mutex_rwait(&s,23,31)==PORT_MUTEX_BLOCKED);
    assert(s.resource_mask==0x80000001u);
    assert(port_mutex_ravail(&s,0)==2);
    assert(p[0].state==PORT_PCB_READY && p[1].state==PORT_PCB_READY);
    assert(p[2].state==PORT_PCB_WAIT_RES);
    assert(port_mutex_ravail(&s,31)==1);
    assert(port_mutex_ravail(&s,31)==0);
    assert(!s.resource_mask && !port_mutex_waiting(&s));
}
int main(void) {
    uint32_t errors=port_mutex_selftest();
    if(errors)fprintf(stderr,"MUTEX selftest mask=0x%08x\n",errors);
    assert(errors==0);
    test_multi_owner();test_resources();
    puts("MUTEX host PASS: nested owners, pending writers, release/retry, IO locks, resource wake, saved priorities");
    return 0;
}
