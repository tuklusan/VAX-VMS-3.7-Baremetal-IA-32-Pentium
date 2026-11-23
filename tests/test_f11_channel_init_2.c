#include "test_f11_channel_cases.h"
/* M185: independent init regression #2. */
int test_f11_channel_init_2(void){
struct m192_env e; m192_setup(&e);struct port_f11_chan_table before=e.table;
assert(port_f11_chan_init(&e.table,0,&e.queue,0)==PORT_F11_CHAN_BAD);
assert(memcmp(&before,&e.table,sizeof(before))==0);
return 1;
}
