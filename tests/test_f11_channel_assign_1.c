#include "test_f11_channel_cases.h"
/* M187: independent assign regression #1. */
int test_f11_channel_assign_1(void){
struct m192_env e; m192_setup(&e);uint32_t h[4]={0};
for(unsigned i=0;i<4;i++)assert(port_f11_chan_assign(&e.table,10+i,&h[i])==PORT_F11_CHAN_OK);
uint32_t old=0xdeadbeefu;
assert(port_f11_chan_assign(&e.table,55,&old)==PORT_F11_CHAN_BUSY);
assert(old==0xdeadbeefu&&e.table.open_count==4u);
return 1;
}
