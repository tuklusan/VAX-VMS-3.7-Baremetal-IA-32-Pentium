#include "dq_ioc_test_common.h"
/* M164: enqueue scenario 1, original source private IOC behavior. */
int test_ioc_enqueue_1(void){
 struct port_dq_ioc_queue q;struct dq_ioc_fixture f;
 dq_ioc_test_setup(&q,&f);
 uint8_t b[512]={0};assert(port_dq_ioc_prepare(&q,1u,2u,b,512u)==PORT_DQ_IOC_OK);assert(port_dq_ioc_enqueue(&q,1u)==PORT_DQ_IOC_OK);assert(q.submitted==1u);
 return 1;
}
