#include "dq_retry_test_common.h"
int test_retry_run_3(void){
struct port_dq_ioc_queue q;struct retry_fixture f;uint8_t a[512];struct port_dq_ioc_report r={6,6,6,6};retry_setup(&q,&f,0,0);retry_add(&q,3,5,a,0);retry_start(&q,3);assert(port_dq_ioc_run(&q,&r)==PORT_DQ_IOC_BUSY&&r.processed==6&&f.calls==0);
return 1;
}
