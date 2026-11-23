#include "dq_retry_test_common.h"
int test_retry_set_retry_0(void){
struct port_dq_ioc_queue q;struct retry_fixture f;uint8_t b[512]={0};retry_setup(&q,&f,0,0);assert(port_dq_ioc_prepare(&q,1,3,b,512)==PORT_DQ_IOC_OK);assert(port_dq_ioc_set_retry(&q,1,3)==PORT_DQ_IOC_OK);assert(q.packets[0].retries_remaining==3&&q.packets[0].retry_limit==3);
return 1;
}
