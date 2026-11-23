#include "f11_cursor_cases.h"
int test_cursor_init_3(void){
 struct cursor_fixture f;cursor_fixture_init(&f);
 f.chain.extents[0].lbn=0x0FFFFFFFu;f.chain.extents[0].blocks=3u;T(port_f11_cursor_init(&f.cursor,&f.volume,&f.chain,0)==PORT_F11_CURSOR_MAP);T(f.cursor.total_bytes==2560u);
 return 0;
}
