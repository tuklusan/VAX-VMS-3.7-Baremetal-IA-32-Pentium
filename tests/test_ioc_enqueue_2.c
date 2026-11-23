#include "dq_ioc_test_common.h"
/* M164: enqueue scenario 2, original source private IOC behavior. */
int test_ioc_enqueue_2(void){
 struct port_dq_ioc_queue q;struct dq_ioc_fixture f;
 dq_ioc_test_setup(&q,&f);
 uint8_t b[512]={0};dq_ioc_test_add(&q,1u,2u,b);assert(port_dq_ioc_enqueue(&q,1u)==PORT_DQ_IOC_BUSY);
 return 1;
}
