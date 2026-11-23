#include "test_f11_channel_cases.h"
/* M191: independent compare regression #2. */
int test_f11_channel_compare_2(void){
struct m192_env e; m192_setup(&e);uint32_t h=0,equal=888;
uint8_t want[300];m192_expected(want,0,300);
assert(port_f11_chan_assign(&e.table,1,&h)==PORT_F11_CHAN_OK);
assert(port_f11_chan_compare(&e.table,2,h,want,300,&equal)==PORT_F11_CHAN_NOCHANNEL);
assert(equal==888&&e.reads==0);
return 1;
}
