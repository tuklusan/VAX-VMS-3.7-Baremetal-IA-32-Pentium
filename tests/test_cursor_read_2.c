#include "f11_cursor_cases.h"
int test_cursor_read_2(void){
 struct cursor_fixture f;cursor_fixture_init(&f);
 T(port_f11_cursor_seek(&f.cursor,500)==PORT_F11_CURSOR_OK);f.fail_at=2u;uint8_t b[32]={0};uint32_t n=0;T(port_f11_cursor_read(&f.cursor,b,32,sizeof b,&n)==PORT_F11_CURSOR_IO);T(n==12u&&f.cursor.position==512u&&f.cursor.reads==1u);
 return 0;
}
