#include "f11_cursor_cases.h"
int test_cursor_seek_1(void){
 struct cursor_fixture f;cursor_fixture_init(&f);
 T(port_f11_cursor_seek(&f.cursor,2560)==PORT_F11_CURSOR_OK);T(port_f11_cursor_seek(&f.cursor,2561)==PORT_F11_CURSOR_EOF);T(f.cursor.position==2560u);
 return 0;
}
