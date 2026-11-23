#include "f11_cursor_cases.h"
int test_cursor_init_0(void){
 struct cursor_fixture f;cursor_fixture_init(&f);
 T(port_f11_cursor_init(&f.cursor,&f.volume,&f.chain,0)==PORT_F11_CURSOR_OK);T(f.cursor.total_bytes==2560u);T(f.cursor.position==0u);
 return 0;
}
