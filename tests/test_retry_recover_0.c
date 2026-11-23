#include "dq_retry_test_common.h"
int test_retry_recover_0(void){
struct port_dq_ioc_queue q;struct retry_fixture f;uint8_t b[512];retry_setup(&q,&f,1,0);retry_add(&q,1,4,b,1);retry_start(&q,1);assert(port_dq_ioc_attempt(&q)==PORT_DQ_IOC_IO);assert(port_dq_ioc_recover(&q)==PORT_DQ_IOC_OK);assert(q.retried==1&&q.packets[q.active].state==PORT_DQ_IOC_ACTIVE);
return 1;
}
