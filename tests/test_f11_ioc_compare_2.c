#include "test_f11_ioc_cases.h"
int test_f11_ioc_compare_2(void){struct case_env e;case_setup(&e);uint8_t ex[4]={0};uint32_t equal=91;return port_f11_ioc_compare(&e.file,ex,2049,&equal)==PORT_F11_IOC_BAD&&equal==91;}
