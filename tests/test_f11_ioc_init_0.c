#include "test_f11_ioc_cases.h"
int test_f11_ioc_init_0(void){struct case_env e;case_setup(&e);return e.file.length==3072u&&e.file.position==0&&e.file.next_tag!=0;}
