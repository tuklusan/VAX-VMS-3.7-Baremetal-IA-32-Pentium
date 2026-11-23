#include "f11_cursor_cases.h"
int test_cursor_init_1(void){
 struct cursor_fixture f;cursor_fixture_init(&f);
 T(port_f11_cursor_init(&f.cursor,&f.volume,&f.chain,2560)==PORT_F11_CURSOR_OK);T(f.cursor.position==2560u);T(!f.requests);
 return 0;
}
