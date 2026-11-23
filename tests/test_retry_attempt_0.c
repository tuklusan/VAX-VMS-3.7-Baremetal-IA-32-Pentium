#include "dq_retry_test_common.h"
int test_retry_attempt_0(void){
struct port_dq_ioc_queue q;struct retry_fixture f;uint8_t b[512]={0};retry_setup(&q,&f,0,0);retry_add(&q,1,7,b,0);retry_start(&q,1);assert(port_dq_ioc_attempt(&q)==PORT_DQ_IOC_OK);assert(q.packets[q.active].attempts==1&&q.reads==1&&b[0]==0);
return 1;
}
