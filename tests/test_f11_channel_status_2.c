#include "test_f11_channel_cases.h"
/* M192: independent status regression #2. */
int test_f11_channel_status_2(void){
struct m192_env e; m192_setup(&e);uint32_t h=0;
struct port_f11_chan_status s={.position=55};
assert(port_f11_chan_assign(&e.table,7,&h)==PORT_F11_CHAN_OK);
assert(port_f11_chan_status(&e.table,8,h,&s)==PORT_F11_CHAN_NOCHANNEL);
assert(s.position==55);
return 1;
}
