#include "f11_cursor_cases.h"
int test_cursor_locate_3(void){
 struct cursor_fixture f;cursor_fixture_init(&f);
 T(port_f11_cursor_seek(&f.cursor,2560)==PORT_F11_CURSOR_OK);uint32_t l=99,w=99;T(port_f11_cursor_locate(&f.cursor,&l,&w)==PORT_F11_CURSOR_EOF);T(l==99u&&w==99u);
 return 0;
}
