#include "f11_cursor_cases.h"
int test_cursor_word_1(void){
 struct cursor_fixture f;cursor_fixture_init(&f);
 T(port_f11_cursor_seek(&f.cursor,511)==PORT_F11_CURSOR_OK);uint16_t v=0;T(port_f11_cursor_word(&f.cursor,&v)==PORT_F11_CURSOR_OK);T(v==((uint16_t)disk_byte(40,511)|((uint16_t)disk_byte(41,0)<<8)));T(f.cursor.reads==2u);
 return 0;
}
