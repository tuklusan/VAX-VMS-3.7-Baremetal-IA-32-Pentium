#include "f11_cursor_cases.h"
int test_cursor_read_1(void){
 struct cursor_fixture f;cursor_fixture_init(&f);
 T(port_f11_cursor_seek(&f.cursor,1535)==PORT_F11_CURSOR_OK);uint8_t b[3]={0};uint32_t n=0;T(port_f11_cursor_read(&f.cursor,b,3,sizeof b,&n)==PORT_F11_CURSOR_OK);T(b[0]==disk_byte(42,511)&&b[1]==disk_byte(80,0));T(f.cursor.reads==2u);
 return 0;
}
