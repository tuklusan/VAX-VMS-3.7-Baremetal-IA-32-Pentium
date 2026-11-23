#include "test_f11_channel_cases.h"
/* M186: independent validate regression #1. */
int test_f11_channel_validate_1(void){
struct m192_env e; m192_setup(&e);e.table.open_count=1;
assert(port_f11_chan_validate(&e.table)==PORT_F11_CHAN_BAD);
return 1;
}
