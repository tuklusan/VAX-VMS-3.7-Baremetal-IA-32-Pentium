#include "f11_cursor_cases.h"
int test_cursor_seek_0(void){
 struct cursor_fixture f;cursor_fixture_init(&f);
 T(port_f11_cursor_seek(&f.cursor,1536)==PORT_F11_CURSOR_OK);T(f.cursor.position==1536u);T(!f.requests);
 return 0;
}
