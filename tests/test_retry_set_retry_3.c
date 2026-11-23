#include "dq_retry_test_common.h"
int test_retry_set_retry_3(void){
struct port_dq_ioc_queue q;struct retry_fixture f;uint8_t b[512];retry_setup(&q,&f,0,0);assert(port_dq_ioc_prepare(&q,1,2,b,512)==PORT_DQ_IOC_OK);assert(port_dq_ioc_set_retry(&q,1,16)==PORT_DQ_IOC_BAD);assert(port_dq_ioc_set_retry(&q,1,15)==PORT_DQ_IOC_OK);assert(port_dq_ioc_set_retry(&q,1,0)==PORT_DQ_IOC_OK);
return 1;
}
