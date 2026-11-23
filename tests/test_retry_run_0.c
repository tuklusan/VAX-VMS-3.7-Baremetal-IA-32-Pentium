#include "dq_retry_test_common.h"
int test_retry_run_0(void){
struct port_dq_ioc_queue q;struct retry_fixture f;uint8_t a[512],b[512],c[512];struct port_dq_ioc_report r={0};retry_setup(&q,&f,0,0);retry_add(&q,3,5,a,0);retry_add(&q,4,6,b,0);retry_add(&q,5,7,c,0);assert(port_dq_ioc_run(&q,&r)==PORT_DQ_IOC_OK&&r.processed==3&&r.succeeded==3&&r.failed==0&&r.reads==3&&c[0]==7);
return 1;
}
