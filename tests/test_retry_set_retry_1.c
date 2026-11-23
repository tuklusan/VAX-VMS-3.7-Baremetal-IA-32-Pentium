#include "dq_retry_test_common.h"
int test_retry_set_retry_1(void){
struct port_dq_ioc_queue q;struct retry_fixture f;retry_setup(&q,&f,0,0);assert(port_dq_ioc_set_retry(&q,10,1)==PORT_DQ_IOC_NOTFOUND);assert(port_dq_ioc_set_retry(&q,0,1)==PORT_DQ_IOC_BAD);
return 1;
}
