#include "dq_retry_test_common.h"
int test_retry_service_1(void){
struct port_dq_ioc_queue q;struct retry_fixture f;uint8_t b[512]={7};struct port_dq_ioc_iosb i;retry_setup(&q,&f,0,1);retry_add(&q,21,3,b,0);assert(port_dq_ioc_service(&q,&i)==PORT_DQ_IOC_IO&&i.status==PORT_DQ_IOC_IO&&b[0]==7&&q.active==PORT_DQ_IOC_DEPTH);
return 1;
}
