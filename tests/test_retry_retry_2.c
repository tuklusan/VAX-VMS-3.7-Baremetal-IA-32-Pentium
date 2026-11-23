#include "dq_retry_test_common.h"
int test_retry_retry_2(void){
struct port_dq_ioc_queue q;struct retry_fixture f;uint8_t b[512];retry_setup(&q,&f,0,0);retry_add(&q,1,5,b,0);retry_start(&q,1);assert(port_dq_ioc_retry(&q)==PORT_DQ_IOC_OK&&f.calls==1&&q.retried==0);
return 1;
}
