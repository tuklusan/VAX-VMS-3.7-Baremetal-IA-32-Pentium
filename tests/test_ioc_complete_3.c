#include "dq_ioc_test_common.h"
/* M167: complete scenario 3, original source private IOC behavior. */
int test_ioc_complete_3(void){
 struct port_dq_ioc_queue q;struct dq_ioc_fixture f;
 dq_ioc_test_setup(&q,&f);
 uint8_t b[512];memset(b,0x55,512);dq_ioc_test_add(&q,6u,3u,b);uint32_t tag=0;assert(port_dq_ioc_start(&q,&tag)==PORT_DQ_IOC_OK);f.fail_at=1u;assert(port_dq_ioc_transfer(&q)==PORT_DQ_IOC_IO);assert(port_dq_ioc_complete(&q,&tag)==PORT_DQ_IOC_IO&&tag==6u&&q.failed==1u&&b[0]==0x55);
 return 1;
}
