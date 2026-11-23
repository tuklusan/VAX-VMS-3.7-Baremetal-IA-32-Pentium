#include "test_f11_channel_cases.h"
/* M187: independent assign regression #3. */
int test_f11_channel_assign_3(void){
struct m192_env e; m192_setup(&e);e.table.next_generation=PORT_F11_CHAN_MAX_GENERATION+1u;
uint32_t h=999;
assert(port_f11_chan_assign(&e.table,1,&h)==PORT_F11_CHAN_BUSY);
assert(h==999&&e.table.open_count==0);
return 1;
}
