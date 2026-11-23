#include "f11_cursor_cases.h"
int test_cursor_skip_3(void){
 struct cursor_fixture f;cursor_fixture_init(&f);
 T(port_f11_cursor_skip(&f.cursor,UINT32_MAX)==PORT_F11_CURSOR_EOF);T(f.cursor.position==0u);
 return 0;
}
