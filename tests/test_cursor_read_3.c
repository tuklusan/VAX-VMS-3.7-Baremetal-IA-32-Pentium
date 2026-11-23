#include "f11_cursor_cases.h"
int test_cursor_read_3(void){
 struct cursor_fixture f;cursor_fixture_init(&f);
 uint8_t b[4]={9,9,9,9};uint32_t n=77;T(port_f11_cursor_read(&f.cursor,b,5,sizeof b,&n)==PORT_F11_CURSOR_BAD);T(n==77u&&f.cursor.position==0u&&f.requests==0u);
 return 0;
}
