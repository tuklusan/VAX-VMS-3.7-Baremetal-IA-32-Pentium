#include "test_f11_channel_cases.h"
/* M188: independent release regression #0. */
int test_f11_channel_release_0(void){
struct m192_env e; m192_setup(&e);uint32_t h=0;
assert(port_f11_chan_assign(&e.table,1,&h)==PORT_F11_CHAN_OK);
assert(port_f11_chan_release(&e.table,1,h)==PORT_F11_CHAN_OK);
assert(e.table.open_count==0&&e.table.releases==1u);
return 1;
}
