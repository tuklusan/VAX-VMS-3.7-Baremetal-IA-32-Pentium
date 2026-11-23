#include "test_f11_ioc_cases.h"
int test_f11_ioc_post_2(void){struct case_env e;case_setup(&e);e.fail_on=1;port_f11_ioc_stage(&e.file);return port_f11_ioc_post(&e.file)==PORT_F11_IOC_IO&&!e.file.pending&&e.queue.failed==1;}
