#include "f11_cursor_cases.h"
int test_cursor_read_0(void){
 struct cursor_fixture f;cursor_fixture_init(&f);
 uint8_t b[3]={0};uint32_t n=999;T(port_f11_cursor_read(&f.cursor,b,3,sizeof b,&n)==PORT_F11_CURSOR_OK);T(n==3u&&f.cursor.position==3u&&f.cursor.reads==1u);T(b[0]==disk_byte(40,0)&&b[2]==disk_byte(40,2));
 return 0;
}
