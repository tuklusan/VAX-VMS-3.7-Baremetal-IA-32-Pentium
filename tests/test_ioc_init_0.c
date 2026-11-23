#include "dq_ioc_test_common.h"
/* M161: init scenario 0, original source private IOC behavior. */
int test_ioc_init_0(void){
 struct port_dq_ioc_queue q;struct dq_ioc_fixture f;
 dq_ioc_test_setup(&q,&f);
 assert(port_dq_ioc_init(0,dq_ioc_test_read,0)==PORT_DQ_IOC_BAD);
 return 1;
}
