#ifndef VMS_DQ_RETRY_TEST_COMMON
#define VMS_DQ_RETRY_TEST_COMMON
#include "dq_ioc_test_common.h"
struct retry_fixture {uint32_t calls,fail_first,always_fail;};
static uint32_t retry_read(void *ctx,uint32_t lbn,uint8_t out[512]){
 struct retry_fixture *f=ctx;f->calls++;
 if(f->always_fail||f->calls<=f->fail_first)return 0;
 for(unsigned i=0;i<512;i++)out[i]=(uint8_t)(lbn+i);
 return 1;
}
static inline void retry_setup(struct port_dq_ioc_queue *q,struct retry_fixture *f,
                         unsigned first,unsigned always){
 *f=(struct retry_fixture){0};f->fail_first=first;f->always_fail=always;
 assert(port_dq_ioc_init(q,retry_read,f)==PORT_DQ_IOC_OK);
}
static inline void retry_add(struct port_dq_ioc_queue *q,unsigned tag,unsigned lbn,
                       uint8_t dst[512],unsigned budget){
 assert(port_dq_ioc_prepare(q,tag,lbn,dst,512u)==PORT_DQ_IOC_OK);
 assert(port_dq_ioc_set_retry(q,tag,(uint8_t)budget)==PORT_DQ_IOC_OK);
 assert(port_dq_ioc_enqueue(q,tag)==PORT_DQ_IOC_OK);
}
static inline void retry_start(struct port_dq_ioc_queue *q,unsigned tag){
 uint32_t got=0;assert(port_dq_ioc_start(q,&got)==PORT_DQ_IOC_OK&&got==tag);
}
#endif
