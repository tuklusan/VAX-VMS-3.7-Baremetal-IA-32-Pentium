#include "f11_cursor_cases.h"
int test_cursor_compare_3(void){
 struct cursor_fixture f;cursor_fixture_init(&f);
 f.fail_at=2u;T(port_f11_cursor_seek(&f.cursor,500)==PORT_F11_CURSOR_OK);uint8_t ref[32]={0};uint32_t n=77;T(port_f11_cursor_compare(&f.cursor,ref,32,&n)==PORT_F11_CURSOR_IO);T(n==0u&&f.cursor.position==512u);
 return 0;
}
