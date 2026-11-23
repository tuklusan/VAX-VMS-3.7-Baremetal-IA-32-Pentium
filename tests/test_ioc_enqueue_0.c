#include "dq_ioc_test_common.h"
/* M164: enqueue scenario 0, original source private IOC behavior. */
int test_ioc_enqueue_0(void){
 struct port_dq_ioc_queue q;struct dq_ioc_fixture f;
 dq_ioc_test_setup(&q,&f);
 assert(port_dq_ioc_enqueue(&q,1u)==PORT_DQ_IOC_NOTFOUND);
 return 1;
}
