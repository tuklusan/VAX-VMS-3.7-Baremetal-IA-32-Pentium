#include "f11_cursor_cases.h"
int test_cursor_init_2(void){
 struct cursor_fixture f;cursor_fixture_init(&f);
 f.chain.extents[1].blocks=0;struct port_f11_cursor saved=f.cursor;T(port_f11_cursor_init(&f.cursor,&f.volume,&f.chain,0)==PORT_F11_CURSOR_MAP);T(memcmp(&saved,&f.cursor,sizeof saved)==0);
 return 0;
}
