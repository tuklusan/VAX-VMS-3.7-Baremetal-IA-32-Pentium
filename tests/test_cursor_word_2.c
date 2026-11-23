#include "f11_cursor_cases.h"
int test_cursor_word_2(void){
 struct cursor_fixture f;cursor_fixture_init(&f);
 T(port_f11_cursor_seek(&f.cursor,2559)==PORT_F11_CURSOR_OK);uint16_t v=0x9999;T(port_f11_cursor_word(&f.cursor,&v)==PORT_F11_CURSOR_EOF);T(v==0x9999u&&f.requests==0u);
 return 0;
}
