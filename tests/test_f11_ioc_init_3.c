#include "test_f11_ioc_cases.h"
int test_f11_ioc_init_3(void){struct case_env e;case_setup(&e);struct port_f11_ioc_file out=e.file;return port_f11_ioc_init(&out,&e.chain,&e.queue,3073)==PORT_F11_IOC_BAD&&out.length==3072;}
