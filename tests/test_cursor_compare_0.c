#include "f11_cursor_cases.h"
int test_cursor_compare_0(void){
 struct cursor_fixture f;cursor_fixture_init(&f);
 uint8_t ref[10];for(unsigned i=0;i<10;i++)ref[i]=disk_byte(40,i);uint32_t n=777;T(port_f11_cursor_compare(&f.cursor,ref,10,&n)==PORT_F11_CURSOR_OK);T(n==10u&&f.cursor.position==10u);
 return 0;
}
