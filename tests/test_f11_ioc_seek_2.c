#include "test_f11_ioc_cases.h"
int test_f11_ioc_seek_2(void){struct case_env e;case_setup(&e);return port_f11_ioc_seek(&e.file,3073)==PORT_F11_IOC_EOF&&e.file.position==0;}
