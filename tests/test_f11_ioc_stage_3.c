#include "test_f11_ioc_cases.h"
int test_f11_ioc_stage_3(void){struct case_env e;case_setup(&e);e.file.next_tag=UINT32_MAX;return port_f11_ioc_stage(&e.file)==PORT_F11_IOC_BAD&&e.reads==0;}
