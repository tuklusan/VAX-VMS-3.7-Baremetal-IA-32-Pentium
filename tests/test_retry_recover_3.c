#include "dq_retry_test_common.h"
int test_retry_recover_3(void){
struct port_dq_ioc_queue q;struct retry_fixture f;retry_setup(&q,&f,0,0);assert(port_dq_ioc_recover(&q)==PORT_DQ_IOC_NOTFOUND);
return 1;
}
