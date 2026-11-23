#include "test_f11_ioc_cases.h"
int test_f11_ioc_read_1(void){struct case_env e;case_setup(&e);uint8_t b[10];memset(b,0x71,sizeof(b));uint32_t done=66;return port_f11_ioc_read(&e.file,b,11,sizeof(b),&done)==PORT_F11_IOC_BAD&&done==66&&b[0]==0x71;}
