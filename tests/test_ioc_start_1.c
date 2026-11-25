#include "dq_ioc_test_common.h"
/* M165: start scenario 1, original source private IOC behavior. */
int test_ioc_start_1(void){
 struct port_dq_ioc_queue q;struct dq_ioc_fixture f;
 dq_ioc_test_setup(&q,&f);
 uint8_t a[512]={0},b[512]={0},c[512]={0};
 assert(port_dq_ioc_prepare(&q,12u,1u,a,sizeof a)==PORT_DQ_IOC_OK);
 assert(port_dq_ioc_prepare(&q,13u,2u,b,sizeof b)==PORT_DQ_IOC_OK);
 assert(port_dq_ioc_prepare(&q,14u,3u,c,sizeof c)==PORT_DQ_IOC_OK);
 assert(port_dq_ioc_set_priority(&q,12u,2u)==PORT_DQ_IOC_OK);
 assert(port_dq_ioc_set_priority(&q,13u,3u)==PORT_DQ_IOC_OK);
 assert(port_dq_ioc_set_priority(&q,14u,3u)==PORT_DQ_IOC_OK);
 assert(port_dq_ioc_enqueue(&q,12u)==PORT_DQ_IOC_OK);
 assert(port_dq_ioc_enqueue(&q,13u)==PORT_DQ_IOC_OK);
 assert(port_dq_ioc_enqueue(&q,14u)==PORT_DQ_IOC_OK);
 uint32_t tag=0u;assert(port_dq_ioc_start(&q,&tag)==PORT_DQ_IOC_OK&&tag==12u);
 assert(port_dq_ioc_transfer(&q)==PORT_DQ_IOC_OK);
 assert(port_dq_ioc_complete(&q,&tag)==PORT_DQ_IOC_OK&&tag==12u);
 assert(port_dq_ioc_start(&q,&tag)==PORT_DQ_IOC_OK&&tag==13u);
 assert(port_dq_ioc_transfer(&q)==PORT_DQ_IOC_OK);
 assert(port_dq_ioc_complete(&q,&tag)==PORT_DQ_IOC_OK&&tag==13u);
 assert(port_dq_ioc_start(&q,&tag)==PORT_DQ_IOC_OK&&tag==14u);
 return 1;
}
