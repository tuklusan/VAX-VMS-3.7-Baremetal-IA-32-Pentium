#ifndef VMS37_F11_IOC_CASES_H
#define VMS37_F11_IOC_CASES_H
#include "vms_f11_ioc.h"
#include <stdint.h>
#include <string.h>
#include <assert.h>
struct case_env { struct port_f11_chain chain; struct port_dq_ioc_queue queue;
 struct port_f11_ioc_file file; uint32_t reads,fail_on; };
static inline uint8_t case_byte(uint32_t l,uint32_t o){return (uint8_t)(l*13u+o*7u+11u);}
static inline uint32_t case_sector(void *ctx,uint32_t l,uint8_t out[512]){
 struct case_env *e=(struct case_env *)ctx;
 e->reads++;
 if(e->fail_on&&e->reads==e->fail_on)return 0u;
 for(unsigned i=0;i<512u;i++)out[i]=case_byte(l,i);
 return 1u;
}
static inline void case_setup(struct case_env *e){
 memset(e,0,sizeof(*e));e->chain.count=2;e->chain.blocks=6;
 e->chain.extents[0].lbn=40;e->chain.extents[0].blocks=3;
 e->chain.extents[1].lbn=80;e->chain.extents[1].blocks=3;
 assert(port_dq_ioc_init(&e->queue,case_sector,e)==PORT_DQ_IOC_OK);
 assert(port_f11_ioc_init(&e->file,&e->chain,&e->queue,3072)==PORT_F11_IOC_OK);
}
static inline uint32_t case_lbn(uint32_t offset){
 uint32_t b=offset/512u;return b<3?40u+b:80u+b-3u;
}
static inline void case_expected(uint8_t *b,uint32_t start,uint32_t n){
 for(uint32_t i=0;i<n;i++)b[i]=case_byte(case_lbn(start+i),(start+i)%512u);
}
#endif
