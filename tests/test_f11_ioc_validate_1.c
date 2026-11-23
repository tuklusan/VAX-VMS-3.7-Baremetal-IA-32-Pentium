#include "test_f11_ioc_cases.h"
int test_f11_ioc_validate_1(void){struct case_env e;case_setup(&e);e.file.position=3073;return port_f11_ioc_validate(&e.file)==PORT_F11_IOC_BAD;}
