#include "f11_cursor_cases.h"
int test_cursor_compare_1(void){
 struct cursor_fixture f;cursor_fixture_init(&f);
 uint8_t ref[12];for(unsigned i=0;i<12;i++)ref[i]=disk_byte(40,i);ref[5]^=1;uint32_t n=99;T(port_f11_cursor_compare(&f.cursor,ref,12,&n)==PORT_F11_CURSOR_DIFFERENT);T(n==5u&&f.cursor.position==12u);
 return 0;
}
