#include "test_f11_channel_cases.h"
/* M192: independent status regression #1. */
int test_f11_channel_status_1(void){
struct m192_env e; m192_setup(&e);uint32_t h=0;
struct port_f11_chan_status s={0};
assert(port_f11_chan_assign(&e.table,7,&h)==PORT_F11_CHAN_OK);
assert(port_f11_chan_seek(&e.table,7,h,512)==PORT_F11_CHAN_OK);
assert(port_f11_chan_status(&e.table,7,h,&s)==PORT_F11_CHAN_OK);
assert(s.position==512&&s.physical_reads==0);
return 1;
}
