#include "test_f11_ioc_cases.h"
int test_f11_ioc_stage_1(void){struct case_env e;case_setup(&e);port_f11_ioc_stage(&e.file);return port_f11_ioc_stage(&e.file)==PORT_F11_IOC_BUSY&&e.queue.submitted==1;}
