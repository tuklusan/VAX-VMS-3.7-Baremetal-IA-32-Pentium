#include "dq_retry_test_common.h"
int test_retry_attempt_3(void){
struct port_dq_ioc_queue q;struct retry_fixture f;uint8_t b[512];retry_setup(&q,&f,0,0);retry_add(&q,1,4,b,0);retry_start(&q,1);q.packets[q.active].attempts=UINT8_MAX;assert(port_dq_ioc_attempt(&q)==PORT_DQ_IOC_CORRUPT);assert(f.calls==0);
return 1;
}
