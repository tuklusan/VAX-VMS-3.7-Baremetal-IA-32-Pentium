#include "dq_retry_test_common.h"
int test_retry_post_0(void){
struct port_dq_ioc_queue q;struct retry_fixture f;uint8_t b[512]={3};struct port_dq_ioc_iosb i;retry_setup(&q,&f,0,0);retry_add(&q,15,8,b,0);retry_start(&q,15);assert(port_dq_ioc_attempt(&q)==PORT_DQ_IOC_OK);assert(port_dq_ioc_post(&q,&i)==PORT_DQ_IOC_OK);assert(q.posted==1&&q.completed==1&&b[0]==8&&i.bytes==512);
return 1;
}
