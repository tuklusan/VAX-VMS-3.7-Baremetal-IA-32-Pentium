#include "test_f11_ioc_cases.h"
int test_f11_ioc_read_0(void){struct case_env e;case_setup(&e);port_f11_ioc_seek(&e.file,500);uint8_t b[700],ex[700];memset(b,0,sizeof(b));case_expected(ex,500,700);uint32_t done=0;return port_f11_ioc_read(&e.file,b,700,sizeof(b),&done)==PORT_F11_IOC_OK&&done==700&&memcmp(b,ex,700)==0&&e.file.position==1200&&e.file.reads==3;}
