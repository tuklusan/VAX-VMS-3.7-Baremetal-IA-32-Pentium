#include "dq_ioc_test_common.h"
/* M164: enqueue scenario 3, original source private IOC behavior. */
int test_ioc_enqueue_3(void){
 struct port_dq_ioc_queue q;struct dq_ioc_fixture f;
 dq_ioc_test_setup(&q,&f);
 uint8_t b[512]={0};dq_ioc_test_add(&q,3u,0u,b);assert(port_dq_ioc_validate(&q)==PORT_DQ_IOC_OK);
 return 1;
}
