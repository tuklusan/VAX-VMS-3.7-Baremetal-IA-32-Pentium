#include "dq_ioc_test_common.h"
/* M162: validate scenario 2, original source private IOC behavior. */
int test_ioc_validate_2(void){
 struct port_dq_ioc_queue q;struct dq_ioc_fixture f;
 dq_ioc_test_setup(&q,&f);
 q.active=0u;assert(port_dq_ioc_validate(&q)==PORT_DQ_IOC_CORRUPT);
 return 1;
}
