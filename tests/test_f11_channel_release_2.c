#include "test_f11_channel_cases.h"
/* M188: independent release regression #2. */
int test_f11_channel_release_2(void){
struct m192_env e; m192_setup(&e);uint32_t h=0,h2=0;
assert(port_f11_chan_assign(&e.table,1,&h)==PORT_F11_CHAN_OK);
assert(port_f11_chan_release(&e.table,1,h)==PORT_F11_CHAN_OK);
assert(port_f11_chan_assign(&e.table,1,&h2)==PORT_F11_CHAN_OK);
assert(h!=h2&&port_f11_chan_release(&e.table,1,h)==PORT_F11_CHAN_NOCHANNEL);
return 1;
}
