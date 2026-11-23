#include "test_f11_channel_cases.h"
/* M185: independent init regression #0. */
int test_f11_channel_init_0(void){
struct m192_env e; m192_setup(&e);assert(port_f11_chan_validate(&e.table)==PORT_F11_CHAN_OK);
return 1;
}
