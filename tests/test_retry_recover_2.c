#include "dq_retry_test_common.h"
int test_retry_recover_2(void){
struct port_dq_ioc_queue q;struct retry_fixture f;uint8_t b[512];retry_setup(&q,&f,0,0);retry_add(&q,1,4,b,2);retry_start(&q,1);assert(port_dq_ioc_recover(&q)==PORT_DQ_IOC_BUSY);assert(q.packets[q.active].retries_remaining==2);
return 1;
}
