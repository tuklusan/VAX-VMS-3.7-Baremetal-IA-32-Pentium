#include "dq_retry_test_common.h"
int test_retry_attempt_1(void){
struct port_dq_ioc_queue q;struct retry_fixture f;uint8_t b[512]={13};retry_setup(&q,&f,1,0);retry_add(&q,1,7,b,0);retry_start(&q,1);assert(port_dq_ioc_attempt(&q)==PORT_DQ_IOC_IO);assert(q.packets[q.active].state==PORT_DQ_IOC_FAILED&&b[0]==13);
return 1;
}
