#include "test_f11_ioc_cases.h"
int test_f11_ioc_seek_1(void){struct case_env e;case_setup(&e);return port_f11_ioc_seek(&e.file,3072)==PORT_F11_IOC_OK&&e.file.position==3072;}
