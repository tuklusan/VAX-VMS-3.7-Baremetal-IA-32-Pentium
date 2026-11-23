#include "dq_retry_test_common.h"
int test_retry_service_3(void){
struct port_dq_ioc_queue q;struct retry_fixture f;uint8_t b[512];struct port_dq_ioc_iosb i;retry_setup(&q,&f,0,0);retry_add(&q,21,3,b,1);retry_start(&q,21);assert(port_dq_ioc_service(&q,&i)==PORT_DQ_IOC_BUSY&&f.calls==0);
return 1;
}
