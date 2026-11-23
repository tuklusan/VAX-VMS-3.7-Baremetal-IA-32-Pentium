#include "dq_retry_test_common.h"
int test_retry_iosb_0(void){
struct port_dq_ioc_queue q;struct retry_fixture f;uint8_t b[512];struct port_dq_ioc_iosb i={0};retry_setup(&q,&f,0,0);retry_add(&q,77,5,b,0);retry_start(&q,77);assert(port_dq_ioc_attempt(&q)==PORT_DQ_IOC_OK);assert(port_dq_ioc_iosb(&q,&i)==PORT_DQ_IOC_OK&&i.tag==77&&i.status==1&&i.bytes==512);
return 1;
}
