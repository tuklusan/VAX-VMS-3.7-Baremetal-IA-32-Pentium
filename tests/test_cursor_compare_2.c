#include "f11_cursor_cases.h"
int test_cursor_compare_2(void){
 struct cursor_fixture f;cursor_fixture_init(&f);
 T(port_f11_cursor_seek(&f.cursor,1534)==PORT_F11_CURSOR_OK);uint8_t ref[8];for(unsigned i=0;i<2;i++)ref[i]=disk_byte(42,510u+i);for(unsigned i=2;i<8;i++)ref[i]=disk_byte(80,i-2u);uint32_t n=0;T(port_f11_cursor_compare(&f.cursor,ref,8,&n)==PORT_F11_CURSOR_OK);T(n==8u&&f.cursor.reads==2u);
 return 0;
}
