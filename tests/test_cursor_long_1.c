#include "f11_cursor_cases.h"
int test_cursor_long_1(void){
 struct cursor_fixture f;cursor_fixture_init(&f);
 T(port_f11_cursor_seek(&f.cursor,1534)==PORT_F11_CURSOR_OK);uint32_t v=0;T(port_f11_cursor_long(&f.cursor,&v)==PORT_F11_CURSOR_OK);T(v==((uint32_t)disk_byte(42,510)|((uint32_t)disk_byte(42,511)<<8)|((uint32_t)disk_byte(80,0)<<16)|((uint32_t)disk_byte(80,1)<<24)));
 return 0;
}
