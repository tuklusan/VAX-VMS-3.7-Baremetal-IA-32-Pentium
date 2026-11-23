#include <assert.h>
#include <stdio.h>
#include "vms_event.h"
#include "vms_pcb.h"
static uint32_t pd[1024] __attribute__((aligned(4096)));
int main(void) {
    struct port_event_cluster f;
    struct port_pcb pcb={0};
    port_event_init(&f);
    assert(port_pcb_create(&pcb,0x370001,pd,0x24000,0x10000)==0);
    assert(port_pcb_activate(&pcb)==0);
    assert(!port_event_satisfied(&f,3,0));
    assert(!port_event_satisfied(&f,3,1));
    assert(port_pcb_block_ef(&pcb,3,1)==0);
    assert(pcb.state==PORT_PCB_WAIT_EF && pcb.wait_mask==3);
    assert(port_pcb_deschedule(&pcb)!=0);
    port_event_set(&f,1);
    assert(port_event_satisfied(&f,3,0));
    assert(!port_event_satisfied(&f,3,1));
    assert(pcb.state==PORT_PCB_WAIT_EF);
    port_event_set(&f,2);
    assert(port_event_satisfied(&f,3,1));
    assert(port_pcb_wake_ef(&pcb)==0);
    assert(pcb.state==PORT_PCB_READY && pcb.wait_mask==0);
    assert(port_pcb_activate(&pcb)==0);
    assert(port_pcb_block_ef(&pcb,0,0)!=0);
    port_event_clear(&f,1);
    assert(!port_event_satisfied(&f,1,0));
    assert(port_event_satisfied(&f,2,0));
    assert(f.sets==2 && f.clears==1);
    puts("M9 event host PASS: OR/AND, clear, wait/ready, PCB lifecycle");
    return 0;
}
