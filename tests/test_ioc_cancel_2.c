#include "dq_ioc_test_common.h"
/* M168: cancel scenario 2, original source private IOC behavior. */
int test_ioc_cancel_2(void){
 struct port_dq_ioc_queue q;struct dq_ioc_fixture f;
 dq_ioc_test_setup(&q,&f);
 uint8_t b[512]={0};dq_ioc_test_add(&q,1u,4u,b);assert(port_dq_ioc_cancel(&q,1u)==PORT_DQ_IOC_OK&&q.submitted==0u);
 return 1;
}
