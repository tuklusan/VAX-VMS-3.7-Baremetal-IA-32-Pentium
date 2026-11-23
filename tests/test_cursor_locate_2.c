#include "f11_cursor_cases.h"
int test_cursor_locate_2(void){
 struct cursor_fixture f;cursor_fixture_init(&f);
 T(port_f11_cursor_seek(&f.cursor,2559)==PORT_F11_CURSOR_OK);uint32_t l=0,w=0;T(port_f11_cursor_locate(&f.cursor,&l,&w)==PORT_F11_CURSOR_OK);T(l==81u&&w==511u);
 return 0;
}
