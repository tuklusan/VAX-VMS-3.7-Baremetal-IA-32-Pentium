#include "dq_ioc_test_common.h"
/* M162: validate scenario 3, original source private IOC behavior. */
int test_ioc_validate_3(void){
 struct port_dq_ioc_queue q;struct dq_ioc_fixture f;
 dq_ioc_test_setup(&q,&f);
 uint8_t b[512]={0};dq_ioc_test_add(&q,1u,4u,b);q.packets[0].lbn=0x10000000u;assert(port_dq_ioc_validate(&q)==PORT_DQ_IOC_CORRUPT);
 return 1;
}
