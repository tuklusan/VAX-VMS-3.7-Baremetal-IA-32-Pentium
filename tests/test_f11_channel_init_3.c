#include "test_f11_channel_cases.h"
/* M185: independent init regression #3. */
int test_f11_channel_init_3(void){
struct m192_env e; m192_setup(&e);struct port_f11_chan_table t={0};
assert(port_f11_chan_init(&t,&e.chain,&e.queue,0)==PORT_F11_CHAN_OK);
assert(t.length==0&&t.open_count==0);
return 1;
}
