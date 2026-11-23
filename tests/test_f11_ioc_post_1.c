#include "test_f11_ioc_cases.h"
int test_f11_ioc_post_1(void){struct case_env e;case_setup(&e);return port_f11_ioc_post(&e.file)==PORT_F11_IOC_BUSY&&e.reads==0;}
