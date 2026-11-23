#include "test_f11_channel_cases.h"
/* M186: independent validate regression #2. */
int test_f11_channel_validate_2(void){
struct m192_env e; m192_setup(&e);uint32_t h=0;
assert(port_f11_chan_assign(&e.table,4,&h)==PORT_F11_CHAN_OK);
e.table.slots[0].owner=0;
assert(port_f11_chan_validate(&e.table)==PORT_F11_CHAN_BAD);
return 1;
}
