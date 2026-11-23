#include "dq_ioc_test_common.h"
/* M165: start scenario 2, original source private IOC behavior. */
int test_ioc_start_2(void){
 struct port_dq_ioc_queue q;struct dq_ioc_fixture f;
 dq_ioc_test_setup(&q,&f);
 uint8_t a[512]={0};dq_ioc_test_add(&q,20u,3u,a);uint32_t tag=0u;assert(port_dq_ioc_start(&q,&tag)==PORT_DQ_IOC_OK);assert(port_dq_ioc_start(&q,&tag)==PORT_DQ_IOC_BUSY);
 return 1;
}
