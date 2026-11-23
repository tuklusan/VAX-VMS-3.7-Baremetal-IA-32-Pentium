#include "dq_retry_test_common.h"
int test_retry_run_1(void){
struct port_dq_ioc_queue q;struct retry_fixture f;uint8_t a[512]={9},b[512]={9};struct port_dq_ioc_report r={0};retry_setup(&q,&f,0,1);retry_add(&q,3,5,a,1);retry_add(&q,4,6,b,0);assert(port_dq_ioc_run(&q,&r)==PORT_DQ_IOC_IO&&r.processed==2&&r.failed==2&&r.reads==3&&a[0]==9&&b[0]==9);
return 1;
}
