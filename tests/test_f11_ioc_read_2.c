#include "test_f11_ioc_cases.h"
int test_f11_ioc_read_2(void){struct case_env e;case_setup(&e);port_f11_ioc_seek(&e.file,3000);uint8_t b[100];uint32_t done=77;return port_f11_ioc_read(&e.file,b,100,sizeof(b),&done)==PORT_F11_IOC_EOF&&done==77&&e.file.position==3000;}
