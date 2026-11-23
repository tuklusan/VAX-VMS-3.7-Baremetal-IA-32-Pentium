#include "dq_ioc_test_common.h"
/* M167: complete scenario 0, original source private IOC behavior. */
int test_ioc_complete_0(void){
 struct port_dq_ioc_queue q;struct dq_ioc_fixture f;
 dq_ioc_test_setup(&q,&f);
 uint32_t tag=0;assert(port_dq_ioc_complete(&q,&tag)==PORT_DQ_IOC_BAD);
 return 1;
}
