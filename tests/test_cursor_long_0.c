#include "f11_cursor_cases.h"
int test_cursor_long_0(void){
 struct cursor_fixture f;cursor_fixture_init(&f);
 uint32_t v=0;T(port_f11_cursor_long(&f.cursor,&v)==PORT_F11_CURSOR_OK);uint32_t want=0;for(unsigned i=0;i<4;i++)want|=(uint32_t)disk_byte(40,i)<<(i*8u);T(v==want);
 return 0;
}
