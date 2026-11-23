#include "test_f11_channel_cases.h"
/* M189: independent seek regression #2. */
int test_f11_channel_seek_2(void){
struct m192_env e; m192_setup(&e);uint32_t h=0;
assert(port_f11_chan_assign(&e.table,1,&h)==PORT_F11_CHAN_OK);
assert(port_f11_chan_seek(&e.table,2,h,30)==PORT_F11_CHAN_NOCHANNEL);
assert(e.table.slots[0].file.position==0);
return 1;
}
