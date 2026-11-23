#include "test_f11_channel_cases.h"
/* M192: independent status regression #3. */
int test_f11_channel_status_3(void){
struct m192_env e; m192_setup(&e);uint32_t h=0,done=0;
uint8_t b[4];struct port_f11_chan_status s={0};
assert(port_f11_chan_assign(&e.table,7,&h)==PORT_F11_CHAN_OK);
assert(port_f11_chan_read(&e.table,7,h,b,4,sizeof(b),&done)==PORT_F11_CHAN_OK);
assert(port_f11_chan_status(&e.table,7,h,&s)==PORT_F11_CHAN_OK);
assert(s.position==4&&s.physical_reads==1&&done==4);
return 1;
}
