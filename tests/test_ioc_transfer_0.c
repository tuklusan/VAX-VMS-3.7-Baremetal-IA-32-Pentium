#include "dq_ioc_test_common.h"
/* M166: transfer scenario 0, original source private IOC behavior. */
int test_ioc_transfer_0(void){
 struct port_dq_ioc_queue q;struct dq_ioc_fixture f;
 dq_ioc_test_setup(&q,&f);
 assert(port_dq_ioc_transfer(&q)==PORT_DQ_IOC_NOTFOUND);
 return 1;
}
