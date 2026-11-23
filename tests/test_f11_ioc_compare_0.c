#include "test_f11_ioc_cases.h"
int test_f11_ioc_compare_0(void){struct case_env e;case_setup(&e);uint8_t ex[800];case_expected(ex,1000,800);port_f11_ioc_seek(&e.file,1000);uint32_t equal=0;return port_f11_ioc_compare(&e.file,ex,800,&equal)==PORT_F11_IOC_OK&&equal==800&&e.file.position==1800;}
