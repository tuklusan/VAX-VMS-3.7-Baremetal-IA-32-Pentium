#include "test_f11_ioc_cases.h"
int test_f11_ioc_locate_0(void){struct case_env e;case_setup(&e);uint32_t l=0,o=9;return port_f11_ioc_locate(&e.file,&l,&o)==PORT_F11_IOC_OK&&l==40&&o==0;}
