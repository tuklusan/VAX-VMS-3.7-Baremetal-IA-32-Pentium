#include "dq_ioc_test_common.h"
/* M168: cancel scenario 0, original source private IOC behavior. */
int test_ioc_cancel_0(void){
 struct port_dq_ioc_queue q;struct dq_ioc_fixture f;
 dq_ioc_test_setup(&q,&f);
 assert(port_dq_ioc_cancel(&q,1u)==PORT_DQ_IOC_NOTFOUND);
 return 1;
}
