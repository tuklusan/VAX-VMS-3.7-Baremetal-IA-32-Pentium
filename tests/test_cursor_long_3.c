#include "f11_cursor_cases.h"
int test_cursor_long_3(void){
 struct cursor_fixture f;cursor_fixture_init(&f);
 f.fail_at=1u;uint32_t v=0xdeadbeef;T(port_f11_cursor_long(&f.cursor,&v)==PORT_F11_CURSOR_IO);T(v==0xdeadbeefu);
 return 0;
}
