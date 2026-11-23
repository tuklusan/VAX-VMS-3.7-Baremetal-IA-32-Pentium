#include "test_f11_channel_cases.h"
/* M190: independent read regression #0. */
int test_f11_channel_read_0(void){
struct m192_env e; m192_setup(&e);uint32_t h=0,done=0;
uint8_t out[700],want[700];assert(port_f11_chan_assign(&e.table,1,&h)==PORT_F11_CHAN_OK);
assert(port_f11_chan_seek(&e.table,1,h,1400)==PORT_F11_CHAN_OK);
assert(port_f11_chan_read(&e.table,1,h,out,700,sizeof(out),&done)==PORT_F11_CHAN_OK);
m192_expected(want,1400,700);assert(done==700&&memcmp(out,want,700)==0&&e.reads==3);
return 1;
}
