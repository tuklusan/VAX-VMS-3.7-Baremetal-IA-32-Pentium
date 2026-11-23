#include "f11_cursor_cases.h"
int test_cursor_locate_0(void){
 struct cursor_fixture f;cursor_fixture_init(&f);
 uint32_t l=0,w=42;T(port_f11_cursor_locate(&f.cursor,&l,&w)==PORT_F11_CURSOR_OK);T(l==40u&&w==0u);
 return 0;
}
