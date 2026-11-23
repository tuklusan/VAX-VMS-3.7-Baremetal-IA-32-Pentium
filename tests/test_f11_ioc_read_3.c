#include "test_f11_ioc_cases.h"
int test_f11_ioc_read_3(void){struct case_env e;case_setup(&e);e.fail_on=2;port_f11_ioc_seek(&e.file,500);uint8_t b[700];memset(b,0x6b,sizeof(b));uint32_t done=0;return port_f11_ioc_read(&e.file,b,700,sizeof(b),&done)==PORT_F11_IOC_IO&&done==12&&e.file.position==500&&b[0]==0x6b;}
