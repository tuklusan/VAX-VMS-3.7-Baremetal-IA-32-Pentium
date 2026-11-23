#include "f11_cursor_cases.h"
int test_cursor_skip_1(void){
 struct cursor_fixture f;cursor_fixture_init(&f);
 T(port_f11_cursor_skip(&f.cursor,1536)==PORT_F11_CURSOR_OK);T(port_f11_cursor_skip(&f.cursor,1025)==PORT_F11_CURSOR_EOF);T(f.cursor.position==1536u);
 return 0;
}
