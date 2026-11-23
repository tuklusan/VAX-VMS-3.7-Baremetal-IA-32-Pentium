#include "f11_cursor_cases.h"
int test_cursor_word_0(void){
 struct cursor_fixture f;cursor_fixture_init(&f);
 uint16_t v=0;T(port_f11_cursor_word(&f.cursor,&v)==PORT_F11_CURSOR_OK);T(v==((uint16_t)disk_byte(40,0)|((uint16_t)disk_byte(40,1)<<8)));
 return 0;
}
