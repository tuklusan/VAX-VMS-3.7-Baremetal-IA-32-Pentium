#include "f11_cursor_cases.h"
int test_cursor_skip_0(void){
 struct cursor_fixture f;cursor_fixture_init(&f);
 T(port_f11_cursor_skip(&f.cursor,511)==PORT_F11_CURSOR_OK);T(f.cursor.position==511u);
 return 0;
}
