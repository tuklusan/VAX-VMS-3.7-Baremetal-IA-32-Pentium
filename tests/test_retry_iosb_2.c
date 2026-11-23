#include "dq_retry_test_common.h"
int test_retry_iosb_2(void){
struct port_dq_ioc_queue q;struct retry_fixture f;uint8_t b[512];struct port_dq_ioc_iosb i={9,9,9};retry_setup(&q,&f,0,0);retry_add(&q,77,5,b,0);retry_start(&q,77);assert(port_dq_ioc_iosb(&q,&i)==PORT_DQ_IOC_BUSY&&i.tag==9);
return 1;
}
