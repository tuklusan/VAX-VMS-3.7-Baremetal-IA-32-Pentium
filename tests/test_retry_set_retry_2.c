#include "dq_retry_test_common.h"
int test_retry_set_retry_2(void){
struct port_dq_ioc_queue q;struct retry_fixture f;uint8_t b[512];retry_setup(&q,&f,0,0);retry_add(&q,1,5,b,1);assert(port_dq_ioc_set_retry(&q,1,2)==PORT_DQ_IOC_BUSY);assert(q.packets[0].retry_limit==1);
return 1;
}
