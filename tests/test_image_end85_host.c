#include "vms_exe_image_end85.h"
#include <assert.h>
#include <stdio.h>
#include <stdint.h>
int main(void){
    assert(((uintptr_t)&port_bug_debug&4095u)==0u);
    assert(sizeof(port_bug_debug)==512);
    assert((uintptr_t)&port_bug_debug.cpu_registers-(uintptr_t)&port_bug_debug==376);
    assert((uintptr_t)&port_bug_debug.general_registers-(uintptr_t)&port_bug_debug==440);
    assert((uintptr_t)&port_bug_debug.flag-(uintptr_t)&port_bug_debug==508);
    assert(port_mmg_pfn_patch.terminator==0);
    for(unsigned i=0;i<16;i++){
        assert(port_mmg_pfn_patch.entries[i].address==0);
        assert(port_mmg_pfn_patch.entries[i].old_opcode==0);
        assert(port_mmg_pfn_patch.entries[i].new_opcode==0);
    }
    assert(port_mmg_pfn_patch_desc.capacity==96);
    assert(port_mmg_pfn_patch_desc.address==port_mmg_pfn_patch.entries);
    assert(((uintptr_t)port_mmg_sys_end&4095u)==0);
    puts("M85 debug and SYS image-end data source checks PASS");
}
