#include "dq_retry_test_common.h"
int test_retry_service_2(void){
struct port_dq_ioc_queue q;struct retry_fixture f;struct port_dq_ioc_iosb i;retry_setup(&q,&f,0,0);assert(port_dq_ioc_service(&q,&i)==PORT_DQ_IOC_NOTFOUND);
return 1;
}
