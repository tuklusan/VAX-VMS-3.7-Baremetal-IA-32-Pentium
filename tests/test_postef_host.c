#include <stdio.h>
#include <assert.h>
#include "vms_postef.h"
static uint32_t pd[1024] __attribute__((aligned(4096)));
static void initpcb(struct port_pcb *p,unsigned pid) {
    assert(port_pcb_create(p,pid,pd,0x24000,0x10000)==0);
    assert(port_pcb_activate(p)==0);
}
static void test_private_and_shared(void) {
    struct port_postef_service s;
    struct port_pcb p[4];
    struct port_event_cluster common;
    port_postef_init(&s);port_event_init(&common);
    assert(port_postef_bind_common(&s,2,&common)==0);
    for(unsigned i=0;i<4;i++){
        initpcb(&p[i],100+i);
        assert(port_postef_register(&s,&p[i])==0);
        assert(port_postef_attach_common(&s,p[i].pid,2)==0);
    }
    assert(port_postef_wait(&s,p[0].pid,2,1u<<7,0)==PORT_POSTEF_WAIT_BLOCKED);
    assert(port_postef_wait(&s,p[1].pid,2,1u<<7,1)==PORT_POSTEF_WAIT_BLOCKED);
    assert(port_postef_wait(&s,p[2].pid,0,1u<<7,0)==PORT_POSTEF_WAIT_BLOCKED);
    unsigned n=0;
    assert(port_postef_post(&s,p[3].pid,71,6,&n)==PORT_POSTEF_WAS_CLEAR && n==2);
    assert(p[0].state==PORT_PCB_READY && p[1].state==PORT_PCB_READY);
    assert(p[2].state==PORT_PCB_WAIT_EF);
    assert(port_postef_post(&s,p[2].pid,7,2,&n)==PORT_POSTEF_WAS_CLEAR && n==1);
    assert(p[2].state==PORT_PCB_READY);
    assert(port_postef_post(&s,p[3].pid,71,6,&n)==PORT_POSTEF_WAS_SET && n==0);
    assert(s.wakes==3 && s.notifications==1);
}
static void test_all_bits_and_backlog(void) {
    struct port_postef_service s;
    struct port_pcb p;
    struct port_event_cluster common;
    port_postef_init(&s);port_event_init(&common);
    initpcb(&p,202);
    assert(port_postef_register(&s,&p)==0);
    assert(port_postef_bind_common(&s,3,&common)==0);
    assert(port_postef_attach_common(&s,p.pid,3)==0);
    assert(port_postef_wait(&s,p.pid,3,0x0000000fu,1)==PORT_POSTEF_WAIT_BLOCKED);
    unsigned n;
    assert(port_postef_post_mask(&s,p.pid,3,0x00000003u,0,&n)==PORT_POSTEF_WAS_CLEAR && n==0);
    assert(port_postef_post_mask(&s,p.pid,3,0x0000000cu,0,&n)==PORT_POSTEF_WAS_CLEAR && n==1);
    assert(port_postef_drain_one(&s)==1);
    assert(port_postef_drain_one(&s)==1);
    assert(s.replica[1].flags==0xFu);
    for(unsigned i=4;i<12;i++) assert(port_postef_post(&s,p.pid,96+i,0,&n)==PORT_POSTEF_WAS_CLEAR);
    assert(port_postef_post(&s,p.pid,108,0,&n)==PORT_POSTEF_SYNC_BACKLOG);
    assert(port_postef_pending(&s)==8 && s.sync_failures==1);
    assert(common.flags&(1u<<12));
    while(port_postef_drain_one(&s)) {}
    assert(s.replica[1].flags & (1u<<11));
    /* Snapshot failure does not erase a previously committed master flag. */
    assert(s.replica[1].flags != common.flags);
}
static void test_validation(void) {
    struct port_postef_service s;
    struct port_pcb p;
    port_postef_init(&s);initpcb(&p,311);
    assert(port_postef_register(&s,&p)==0);
    unsigned n=77;
    assert(port_postef_post(&s,311,96,0,&n)==PORT_POSTEF_UNASSIGNED);
    assert(port_postef_post(&s,311,150,0,&n)==PORT_POSTEF_BAD_CLUSTER);
    assert(port_postef_post(&s,311,3,99,&n)==PORT_POSTEF_INVALID);
    assert(port_postef_post_mask(&s,311,4,1,0,&n)==PORT_POSTEF_BAD_CLUSTER);
    assert(port_postef_wait(&s,311,0,0,0)==PORT_POSTEF_INVALID);
    assert(port_postef_wait(&s,999,0,2,0)==PORT_POSTEF_BAD_PID);
    assert(port_postef_bind_common(&s,0,0)==PORT_POSTEF_BAD_CLUSTER);
    assert(port_postef_register(&s,&p)==PORT_POSTEF_INVALID);
    assert(port_postef_unregister(&s,311)==0);
}
int main(void) {
    assert(port_postef_selftest()==0);
    test_private_and_shared();
    test_all_bits_and_backlog();
    test_validation();
    puts("POSTEF host PASS: cluster assignment, PID, WASSET/CLR, AND/OR multi-wake, priority, common sync, backlog");
    return 0;
}
