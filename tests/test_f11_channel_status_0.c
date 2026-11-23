#include "test_f11_channel_cases.h"
/* M192: independent status regression #0. */
int test_f11_channel_status_0(void){
struct m192_env e; m192_setup(&e);uint32_t h=0;
struct port_f11_chan_status s={0};
assert(port_f11_chan_assign(&e.table,77,&h)==PORT_F11_CHAN_OK);
assert(port_f11_chan_status(&e.table,77,h,&s)==PORT_F11_CHAN_OK);
assert(s.owner==77&&s.handle==h&&s.length==3072&&s.position==0);
return 1;
}
