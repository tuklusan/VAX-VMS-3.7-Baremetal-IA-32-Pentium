#include "test_f11_ioc_cases.h"
int test_f11_ioc_seek_0(void){struct case_env e;case_setup(&e);return port_f11_ioc_seek(&e.file,2048)==PORT_F11_IOC_OK&&e.file.position==2048;}
