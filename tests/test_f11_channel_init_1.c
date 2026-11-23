#include "test_f11_channel_cases.h"
/* M185: independent init regression #1. */
int test_f11_channel_init_1(void){
struct m192_env e; m192_setup(&e);struct port_f11_chan_table old=e.table;
assert(port_f11_chan_init(&e.table,&e.chain,&e.queue,3073)==PORT_F11_CHAN_BAD);
assert(memcmp(&old,&e.table,sizeof(old))==0);
return 1;
}
