#include "test_f11_channel_cases.h"
/* M187: independent assign regression #2. */
int test_f11_channel_assign_2(void){
struct m192_env e; m192_setup(&e);uint32_t h=0x77889900u;
assert(port_f11_chan_assign(&e.table,0,&h)==PORT_F11_CHAN_BAD);
assert(h==0x77889900u&&e.table.open_count==0);
return 1;
}
