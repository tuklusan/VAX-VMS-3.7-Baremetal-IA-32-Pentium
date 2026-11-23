#include "test_f11_ioc_cases.h"
int test_f11_ioc_locate_1(void){struct case_env e;case_setup(&e);port_f11_ioc_seek(&e.file,1536);uint32_t l=0,o=9;return port_f11_ioc_locate(&e.file,&l,&o)==PORT_F11_IOC_OK&&l==80&&o==0;}
