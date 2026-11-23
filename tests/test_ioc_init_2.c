#include "dq_ioc_test_common.h"
/* M161: init scenario 2, original source private IOC behavior. */
int test_ioc_init_2(void){
 struct port_dq_ioc_queue q;struct dq_ioc_fixture f;
 dq_ioc_test_setup(&q,&f);
 assert(q.active==PORT_DQ_IOC_DEPTH&&port_dq_ioc_validate(&q)==PORT_DQ_IOC_OK);
 return 1;
}
