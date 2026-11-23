#include "test_f11_channel_cases.h"
/* M190: independent read regression #2. */
int test_f11_channel_read_2(void){
struct m192_env e; m192_setup(&e);uint32_t h=0,done=78;
uint8_t out[16];memset(out,0xa5,sizeof(out));
assert(port_f11_chan_assign(&e.table,1,&h)==PORT_F11_CHAN_OK);
assert(port_f11_chan_read(&e.table,1,h,out,17,sizeof(out),&done)==PORT_F11_CHAN_BAD);
assert(done==78&&out[0]==0xa5&&e.reads==0);
return 1;
}
