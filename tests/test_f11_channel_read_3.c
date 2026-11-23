#include "test_f11_channel_cases.h"
/* M190: independent read regression #3. */
int test_f11_channel_read_3(void){
struct m192_env e; m192_setup(&e);uint32_t h=0,done=0;
uint8_t out[700];memset(out,0xa5,sizeof(out));e.fail_at=2;
assert(port_f11_chan_assign(&e.table,1,&h)==PORT_F11_CHAN_OK);
assert(port_f11_chan_read(&e.table,1,h,out,700,sizeof(out),&done)==PORT_F11_CHAN_IO);
assert(out[0]==0xa5&&e.table.slots[0].file.position==0&&done==512u);
return 1;
}
