#include "vms_exe_gettim.h"
#include <stdint.h>
#include <assert.h>
#include <stdio.h>
struct slot { uint64_t value; };
static int write_slot(void *ctx,uint32_t address,const uint32_t words[2]) {
    struct slot *s=ctx;
    if(address!=8u)return 0;
    s->value=((uint64_t)words[1]<<32)|words[0];
    return 1;
}
int main(void) {
    struct port_exe_systime clock={0};
    struct slot slot={0};
    port_exe_systime_store(&clock,UINT64_C(0x00000010ffffff00));
    assert(port_exe_gettim(&clock,8,write_slot,&slot)==PORT_SS_NORMAL);
    assert(slot.value==UINT64_C(0x00000010ffffff00));
    port_exe_systime_tick(&clock);
    assert(port_exe_gettim(&clock,8,write_slot,&slot)==PORT_SS_NORMAL);
    assert(slot.value==UINT64_C(0x00000011000185a0));
    for(unsigned i=0;i<99;i++)port_exe_systime_tick(&clock);
    assert(port_exe_gettim(&clock,8,write_slot,&slot)==PORT_SS_NORMAL);
    assert(slot.value==UINT64_C(0x0000001100989580));
    assert(port_exe_gettim(&clock,0,write_slot,&slot)==PORT_SS_ACCVIO);
    puts("PIT executive system clock tests passed");
    return 0;
}
