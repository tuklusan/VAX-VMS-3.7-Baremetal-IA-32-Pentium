#include "test_f11_ioc_cases.h"
int test_f11_ioc_post_0(void){struct case_env e;case_setup(&e);port_f11_ioc_stage(&e.file);return port_f11_ioc_post(&e.file)==PORT_F11_IOC_OK&&!e.file.pending&&e.file.reads==1&&e.queue.posted==1;}
