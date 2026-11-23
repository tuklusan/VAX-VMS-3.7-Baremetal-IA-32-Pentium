#include "dq_ioc_test_common.h"
/* M161: init scenario 3, original source private IOC behavior. */
int test_ioc_init_3(void){
 struct port_dq_ioc_queue q;struct dq_ioc_fixture f;
 dq_ioc_test_setup(&q,&f);
 uint8_t b[512]={0};dq_ioc_test_add(&q,1u,3u,b);assert(port_dq_ioc_init(&q,dq_ioc_test_read,&f)==PORT_DQ_IOC_OK);assert(q.submitted==0u&&q.active==PORT_DQ_IOC_DEPTH);
 return 1;
}
