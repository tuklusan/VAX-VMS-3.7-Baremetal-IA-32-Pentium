#include "test_f11_channel_cases.h"
/* M186: independent validate regression #3. */
int test_f11_channel_validate_3(void){
struct m192_env e; m192_setup(&e);e.table.next_generation=0;
assert(port_f11_chan_validate(&e.table)==PORT_F11_CHAN_BAD);
return 1;
}
