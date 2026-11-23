#include "test_f11_ioc_cases.h"
int test_f11_ioc_compare_1(void){struct case_env e;case_setup(&e);uint8_t ex[48];case_expected(ex,128,48);ex[31]^=1;port_f11_ioc_seek(&e.file,128);uint32_t equal=99;return port_f11_ioc_compare(&e.file,ex,48,&equal)==PORT_F11_IOC_DIFFERENT&&equal==31;}
