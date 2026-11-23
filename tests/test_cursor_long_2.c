#include "f11_cursor_cases.h"
int test_cursor_long_2(void){
 struct cursor_fixture f;cursor_fixture_init(&f);
 T(port_f11_cursor_seek(&f.cursor,2557)==PORT_F11_CURSOR_OK);uint32_t v=0xdeadbeef;T(port_f11_cursor_long(&f.cursor,&v)==PORT_F11_CURSOR_EOF);T(v==0xdeadbeefu);
 return 0;
}
