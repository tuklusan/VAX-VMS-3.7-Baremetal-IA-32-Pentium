#include "dq_ioc_test_common.h"
/* M165: start scenario 1, original source private IOC behavior. */
int test_ioc_start_1(void){
 struct port_dq_ioc_queue q;struct dq_ioc_fixture f;
 dq_ioc_test_setup(&q,&f);
 uint8_t a[512]={0},b[512]={0};dq_ioc_test_add(&q,12u,1u,a);dq_ioc_test_add(&q,13u,2u,b);uint32_t tag=0u;assert(port_dq_ioc_start(&q,&tag)==PORT_DQ_IOC_OK&&tag==12u);
 return 1;
}
