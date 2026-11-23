#include "test_f11_channel_cases.h"
/* M189: independent seek regression #1. */
int test_f11_channel_seek_1(void){
struct m192_env e; m192_setup(&e);uint32_t h=0;
assert(port_f11_chan_assign(&e.table,1,&h)==PORT_F11_CHAN_OK);
assert(port_f11_chan_seek(&e.table,1,h,3072)==PORT_F11_CHAN_OK);
assert(port_f11_chan_seek(&e.table,1,h,3073)==PORT_F11_CHAN_EOF);
assert(e.table.slots[0].file.position==3072);
return 1;
}
