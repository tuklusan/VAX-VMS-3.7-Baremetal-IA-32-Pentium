#include "test_f11_ioc_cases.h"
int test_f11_ioc_validate_2(void){struct case_env e;case_setup(&e);e.file.pending=1;e.file.pending_tag=99;return port_f11_ioc_validate(&e.file)==PORT_F11_IOC_BAD;}
