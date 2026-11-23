#include "test_f11_ioc_cases.h"
int test_f11_ioc_compare_3(void){struct case_env e;case_setup(&e);e.fail_on=1;uint8_t ex[20]={0};uint32_t equal=65;return port_f11_ioc_compare(&e.file,ex,20,&equal)==PORT_F11_IOC_IO&&equal==65&&e.file.position==0;}
