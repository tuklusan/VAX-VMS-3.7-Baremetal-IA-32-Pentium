#include "dq_ioc_test_common.h"
/* M165: start scenario 0, original source private IOC behavior. */
int test_ioc_start_0(void){
 struct port_dq_ioc_queue q;struct dq_ioc_fixture f;
 dq_ioc_test_setup(&q,&f);
 uint32_t tag=123u;assert(port_dq_ioc_start(&q,&tag)==PORT_DQ_IOC_NOTFOUND&&tag==123u);
 return 1;
}
