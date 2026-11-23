#include "test_f11_channel_cases.h"
/* M191: independent compare regression #3. */
int test_f11_channel_compare_3(void){
struct m192_env e; m192_setup(&e);uint32_t h=0,equal=888;
uint8_t want[300];m192_expected(want,0,300);e.fail_at=1;
assert(port_f11_chan_assign(&e.table,1,&h)==PORT_F11_CHAN_OK);
assert(port_f11_chan_compare(&e.table,1,h,want,300,&equal)==PORT_F11_CHAN_IO);
assert(equal==888&&e.table.slots[0].file.position==0);
return 1;
}
