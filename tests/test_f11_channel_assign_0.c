#include "test_f11_channel_cases.h"
/* M187: independent assign regression #0. */
int test_f11_channel_assign_0(void){
struct m192_env e; m192_setup(&e);uint32_t h=0;
assert(port_f11_chan_assign(&e.table,3,&h)==PORT_F11_CHAN_OK);
assert(h==257u&&e.table.open_count==1u&&e.table.assignments==1u);
return 1;
}
