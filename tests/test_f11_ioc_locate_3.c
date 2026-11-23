#include "test_f11_ioc_cases.h"
int test_f11_ioc_locate_3(void){struct case_env e;case_setup(&e);port_f11_ioc_seek(&e.file,3072);uint32_t l=77,o=88;return port_f11_ioc_locate(&e.file,&l,&o)==PORT_F11_IOC_EOF&&l==77&&o==88;}
