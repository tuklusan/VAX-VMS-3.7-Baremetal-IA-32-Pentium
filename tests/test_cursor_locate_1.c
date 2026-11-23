#include "f11_cursor_cases.h"
int test_cursor_locate_1(void){
 struct cursor_fixture f;cursor_fixture_init(&f);
 T(port_f11_cursor_seek(&f.cursor,1536)==PORT_F11_CURSOR_OK);uint32_t l=0,w=999;T(port_f11_cursor_locate(&f.cursor,&l,&w)==PORT_F11_CURSOR_OK);T(l==80u&&w==0u);
 return 0;
}
