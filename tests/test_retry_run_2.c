#include "dq_retry_test_common.h"
int test_retry_run_2(void){
struct port_dq_ioc_queue q;struct retry_fixture f;struct port_dq_ioc_report r={4,4,4,4};retry_setup(&q,&f,0,0);assert(port_dq_ioc_run(&q,&r)==PORT_DQ_IOC_OK&&r.processed==0&&r.reads==0);
return 1;
}
