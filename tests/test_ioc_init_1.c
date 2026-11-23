#include "dq_ioc_test_common.h"
/* M161: init scenario 1, original source private IOC behavior. */
int test_ioc_init_1(void){
 struct port_dq_ioc_queue q;struct dq_ioc_fixture f;
 dq_ioc_test_setup(&q,&f);
 assert(port_dq_ioc_init(&q,0,&f)==PORT_DQ_IOC_BAD);
 return 1;
}
