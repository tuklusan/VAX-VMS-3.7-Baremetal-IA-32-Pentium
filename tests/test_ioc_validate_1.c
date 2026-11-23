#include "dq_ioc_test_common.h"
/* M162: validate scenario 1, original source private IOC behavior. */
int test_ioc_validate_1(void){
 struct port_dq_ioc_queue q;struct dq_ioc_fixture f;
 dq_ioc_test_setup(&q,&f);
 q.active=9u;assert(port_dq_ioc_validate(&q)==PORT_DQ_IOC_BAD);
 return 1;
}
