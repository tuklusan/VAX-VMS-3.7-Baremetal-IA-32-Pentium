#include "test_f11_ioc_cases.h"
int test_f11_ioc_stage_2(void){struct case_env e;case_setup(&e);port_f11_ioc_seek(&e.file,3072);return port_f11_ioc_stage(&e.file)==PORT_F11_IOC_EOF&&!e.file.pending;}
