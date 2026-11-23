#include "f11_cursor_cases.h"
int test_cursor_seek_2(void){
 struct cursor_fixture f;cursor_fixture_init(&f);
 T(port_f11_cursor_seek(&f.cursor,700)==PORT_F11_CURSOR_OK);T(port_f11_cursor_seek(&f.cursor,0)==PORT_F11_CURSOR_OK);T(f.cursor.position==0u);
 return 0;
}
