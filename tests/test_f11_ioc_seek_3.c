#include "test_f11_ioc_cases.h"
int test_f11_ioc_seek_3(void){struct case_env e;case_setup(&e);e.file.pending=1;return port_f11_ioc_seek(&e.file,300)==PORT_F11_IOC_BUSY&&e.file.position==0;}
