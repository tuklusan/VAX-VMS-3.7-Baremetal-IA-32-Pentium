#include "dq_ioc_test_common.h"
/* M163: prepare scenario 0, original source private IOC behavior. */
int test_ioc_prepare_0(void){
 struct port_dq_ioc_queue q;struct dq_ioc_fixture f;
 dq_ioc_test_setup(&q,&f);
 uint8_t b[512]={0};assert(port_dq_ioc_prepare(&q,0u,1u,b,512u)==PORT_DQ_IOC_BAD);
 return 1;
}
