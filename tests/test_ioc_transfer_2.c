#include "dq_ioc_test_common.h"
/* M166: transfer scenario 2, original source private IOC behavior. */
int test_ioc_transfer_2(void){
 struct port_dq_ioc_queue q;struct dq_ioc_fixture f;
 dq_ioc_test_setup(&q,&f);
 uint8_t b[512];memset(b,0x7d,512);dq_ioc_test_add(&q,1u,8u,b);uint32_t tag=0;assert(port_dq_ioc_start(&q,&tag)==PORT_DQ_IOC_OK);f.fail_at=1u;assert(port_dq_ioc_transfer(&q)==PORT_DQ_IOC_IO&&b[0]==0x7d);
 return 1;
}
