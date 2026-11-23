#include "dq_retry_test_common.h"
int test_retry_service_0(void){
struct port_dq_ioc_queue q;struct retry_fixture f;uint8_t b[512];struct port_dq_ioc_iosb i;retry_setup(&q,&f,1,0);retry_add(&q,21,3,b,2);assert(port_dq_ioc_service(&q,&i)==PORT_DQ_IOC_OK&&i.tag==21&&i.bytes==512&&f.calls==2);
return 1;
}
