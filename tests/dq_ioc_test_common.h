#ifndef VMS37_DQ_IOC_TEST_COMMON_H
#define VMS37_DQ_IOC_TEST_COMMON_H
#include "vms_dq_ioc.h"
#include <stdint.h>
#include <stddef.h>
#include <assert.h>
#include <string.h>
struct dq_ioc_fixture { uint32_t count,fail_at,base; };
static uint32_t dq_ioc_test_read(void *c,uint32_t lbn,uint8_t out[512]){
 struct dq_ioc_fixture *f=c;
 ++f->count;
 if(f->fail_at&&f->count>=f->fail_at)return 0u;
 for(unsigned i=0;i<512;i++)out[i]=(uint8_t)(lbn+i+f->base);
 return PORT_DQ_IOC_OK;
}
static inline void dq_ioc_test_setup(struct port_dq_ioc_queue *q,
                             struct dq_ioc_fixture *f){
 *f=(struct dq_ioc_fixture){0};
 assert(port_dq_ioc_init(q,dq_ioc_test_read,f)==PORT_DQ_IOC_OK);
}
static inline void dq_ioc_test_add(struct port_dq_ioc_queue *q,
                           unsigned tag,unsigned lbn,uint8_t out[512]){
 assert(port_dq_ioc_prepare(q,tag,lbn,out,512u)==PORT_DQ_IOC_OK);
 assert(port_dq_ioc_enqueue(q,tag)==PORT_DQ_IOC_OK);
}
static inline void dq_ioc_test_run(struct port_dq_ioc_queue *q,uint32_t expected){
 uint32_t tag=0;
 assert(port_dq_ioc_start(q,&tag)==PORT_DQ_IOC_OK);
 assert(tag==expected);
 assert(port_dq_ioc_transfer(q)==PORT_DQ_IOC_OK);
 assert(port_dq_ioc_complete(q,&tag)==PORT_DQ_IOC_OK);
 assert(tag==expected);
}
#endif
