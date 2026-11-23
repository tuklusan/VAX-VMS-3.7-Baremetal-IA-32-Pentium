#include "dq_ioc_test_common.h"
/* M168: cancel scenario 1, original source private IOC behavior. */
int test_ioc_cancel_1(void){
 struct port_dq_ioc_queue q;struct dq_ioc_fixture f;
 dq_ioc_test_setup(&q,&f);
 uint8_t b[512]={0};assert(port_dq_ioc_prepare(&q,1u,4u,b,512u)==PORT_DQ_IOC_OK);assert(port_dq_ioc_cancel(&q,1u)==PORT_DQ_IOC_OK);assert(port_dq_ioc_validate(&q)==PORT_DQ_IOC_OK);
 return 1;
}
