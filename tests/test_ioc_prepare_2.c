#include "dq_ioc_test_common.h"
/* M163: prepare scenario 2, original source private IOC behavior. */
int test_ioc_prepare_2(void){
 struct port_dq_ioc_queue q;struct dq_ioc_fixture f;
 dq_ioc_test_setup(&q,&f);
 uint8_t b[512]={0};assert(port_dq_ioc_prepare(&q,1u,5u,b,511u)==PORT_DQ_IOC_BAD);assert(q.order==0u);
 return 1;
}
