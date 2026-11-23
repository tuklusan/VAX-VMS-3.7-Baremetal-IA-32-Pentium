#include "dq_ioc_test_common.h"
/* M162: validate scenario 0, original source private IOC behavior. */
int test_ioc_validate_0(void){
 struct port_dq_ioc_queue q;struct dq_ioc_fixture f;
 dq_ioc_test_setup(&q,&f);
 assert(port_dq_ioc_validate(0)==PORT_DQ_IOC_BAD);
 return 1;
}
