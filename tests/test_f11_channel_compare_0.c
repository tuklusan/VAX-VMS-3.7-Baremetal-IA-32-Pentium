#include "test_f11_channel_cases.h"
/* M191: independent compare regression #0. */
int test_f11_channel_compare_0(void){
struct m192_env e; m192_setup(&e);uint32_t h=0,equal=0;
uint8_t want[300];m192_expected(want,0,300);
assert(port_f11_chan_assign(&e.table,1,&h)==PORT_F11_CHAN_OK);
assert(port_f11_chan_compare(&e.table,1,h,want,300,&equal)==PORT_F11_CHAN_OK);
assert(equal==300&&e.reads==1);
return 1;
}
