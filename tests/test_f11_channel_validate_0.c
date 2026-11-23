#include "test_f11_channel_cases.h"
/* M186: independent validate regression #0. */
int test_f11_channel_validate_0(void){
struct m192_env e; m192_setup(&e);uint32_t h=0;
assert(port_f11_chan_assign(&e.table,1,&h)==PORT_F11_CHAN_OK);
assert(port_f11_chan_validate(&e.table)==PORT_F11_CHAN_OK);
return 1;
}
