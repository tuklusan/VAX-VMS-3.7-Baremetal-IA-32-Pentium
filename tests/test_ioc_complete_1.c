#include "dq_ioc_test_common.h"
/* M167: complete scenario 1, original source private IOC behavior. */
int test_ioc_complete_1(void){
 struct port_dq_ioc_queue q;struct dq_ioc_fixture f;
 dq_ioc_test_setup(&q,&f);
 uint8_t b[512]={0};dq_ioc_test_add(&q,3u,5u,b);uint32_t tag=0;assert(port_dq_ioc_start(&q,&tag)==PORT_DQ_IOC_OK);assert(port_dq_ioc_complete(&q,&tag)==PORT_DQ_IOC_BUSY);
 return 1;
}
