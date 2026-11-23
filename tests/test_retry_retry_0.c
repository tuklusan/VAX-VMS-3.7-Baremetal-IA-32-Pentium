#include "dq_retry_test_common.h"
int test_retry_retry_0(void){
struct port_dq_ioc_queue q;struct retry_fixture f;uint8_t b[512]={8};retry_setup(&q,&f,2,0);retry_add(&q,1,6,b,2);retry_start(&q,1);assert(port_dq_ioc_retry(&q)==PORT_DQ_IOC_OK);assert(f.calls==3&&q.retried==2&&q.packets[q.active].attempts==3&&b[0]==8);
return 1;
}
