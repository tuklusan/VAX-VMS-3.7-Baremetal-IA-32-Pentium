#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "vms_exe_scbvector.h"
int main(void){
    struct port_exe_scb_slot v[PORT_SCB_ENTRIES];
    port_exe_scb_init(v);
    for(unsigned i=0;i<PORT_SCB_ENTRIES;i++)assert(v[i].vax_byte_offset==i*4);
    assert(v[0].unused && v[0].interrupt_stack);
    assert(!strcmp(v[0x24/4].original_handler,"MMG$PAGEFAULT"));
    assert(v[0x24/4].pc_idt_vector==14 && !v[0x24/4].interrupt_stack);
    assert(!strcmp(v[0x08/4].original_handler,"EXE$KERSTKNV") && v[0x08/4].interrupt_stack);
    assert(!strcmp(v[0x8c/4].original_handler,"SCH$RESCHED"));
    assert(!strcmp(v[0x9c/4].original_handler,"EXE$SWTIMINT"));
    assert(!strcmp(v[0xc0/4].original_handler,"EXE$HWCLKINT"));
    assert(v[0xc0/4].pc_idt_vector==32 && v[0xc0/4].interrupt_stack);
    assert(!strcmp(v[0xf8/4].original_handler,"CON$INTDISI"));
    assert(!strcmp(v[0xfc/4].original_handler,"CON$INTDISO"));
    assert(v[0xf8/4].pc_idt_vector==v[0xfc/4].pc_idt_vector);
    for(unsigned off=256;off<512;off+=4)assert(v[off/4].unused && v[off/4].interrupt_stack);
    assert(port_exe_scb_get(v,511)==NULL && port_exe_scb_get(v,512)==NULL);
    assert(port_exe_scb_get(v,0x9c)==v+0x9c/4);
    puts("PASS: SCBVECTOR fault, soft interrupt and 64 nexus slots");
}
