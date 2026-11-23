#include "test_f11_ioc_cases.h"
int test_f11_ioc_validate_0(void){struct case_env e;case_setup(&e);return port_f11_ioc_validate(&e.file)==PORT_F11_IOC_OK;}
