#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include "vms_pcb.h"
static uint32_t aligned_directory[1024] __attribute__((aligned(4096)));
int main(void) {
    struct port_pcb p={0};
    assert(port_pcb_create(&p,0,aligned_directory,0x8000,0x4000)<0);
    assert(port_pcb_create(&p,37,aligned_directory,0x8000,0x4000)==0);
    assert(port_pcb_consistent(&p));
    assert(p.state==PORT_PCB_READY && p.ast.pid==37);
    assert(port_pcb_bind_kernel_stack(&p,0x32000u,0x33000u)==0);
    assert(port_pcb_bind_kernel_stack(&p,0x32001u,0x33001u)<0);
    assert(port_pcb_bind_kernel_stack(&p,0x32000u,0x34000u)<0);
    assert(p.kernel_stack_base==0x32000u && p.kernel_stack_top==0x33000u);
    assert(port_pcb_consistent(&p));
    port_pcb_tick(&p); /* tick before RUNNING is ignored */
    assert(p.cpu_ticks==0);
    assert(port_pcb_activate(&p)==0 && p.dispatches==1);
    assert(port_pcb_bind_kernel_stack(&p,0x32000u,0x33000u)<0);
    port_pcb_tick(&p);
    assert(p.cpu_ticks==1);
    assert(port_pcb_deschedule(&p)==0 && p.state==PORT_PCB_READY);
    assert(port_pcb_activate(&p)==0 && p.dispatches==2);
    p.started=1;
    assert(port_pcb_exit(&p)==0 && p.state==PORT_PCB_EXITED);
    assert(port_pcb_activate(&p)<0 && port_pcb_consistent(&p));
    puts("PCB lifecycle PASS");
    return 0;
}
