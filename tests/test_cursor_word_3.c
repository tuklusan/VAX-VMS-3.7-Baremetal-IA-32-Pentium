#include "f11_cursor_cases.h"
int test_cursor_word_3(void){
 struct cursor_fixture f;cursor_fixture_init(&f);
 f.fail_at=1u;uint16_t v=0xbeef;T(port_f11_cursor_word(&f.cursor,&v)==PORT_F11_CURSOR_IO);T(v==0xbeefu);
 return 0;
}
