#include "dq_ioc_test_common.h"
/* M163: prepare scenario 3, original source private IOC behavior. */
int test_ioc_prepare_3(void){
 struct port_dq_ioc_queue q;struct dq_ioc_fixture f;
 dq_ioc_test_setup(&q,&f);
 uint8_t b[9][512]={{0}};for(unsigned i=0;i<8;i++)assert(port_dq_ioc_prepare(&q,i+1u,i,b[i],512u)==PORT_DQ_IOC_OK);assert(port_dq_ioc_prepare(&q,9u,8u,b[8],512u)==PORT_DQ_IOC_FULL);assert(port_dq_ioc_prepare(&q,1u,2u,b[8],512u)==PORT_DQ_IOC_BUSY);
 return 1;
}
