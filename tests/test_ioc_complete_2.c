#include "dq_ioc_test_common.h"
/* M167: complete scenario 2, original source private IOC behavior. */
int test_ioc_complete_2(void){
 struct port_dq_ioc_queue q;struct dq_ioc_fixture f;
 dq_ioc_test_setup(&q,&f);
 uint8_t b[512]={0};dq_ioc_test_add(&q,4u,5u,b);dq_ioc_test_run(&q,4u);assert(b[0]==5u&&b[511]==(uint8_t)(5u+511u)&&q.completed==1u);
 return 1;
}
