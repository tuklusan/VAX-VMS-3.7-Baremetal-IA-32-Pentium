#ifndef M192_CHANNEL_CASES_H
#define M192_CHANNEL_CASES_H
#include "vms_f11_channel.h"
#include <assert.h>
#include <string.h>
struct m192_env {
 struct port_f11_chain chain;
 struct port_dq_ioc_queue queue;
 struct port_f11_chan_table table;
 uint32_t reads,fail_at;
};
static inline uint8_t m192_byte(uint32_t lbn,uint32_t i){
 return (uint8_t)(lbn*13u+i*7u+11u);
}
static inline uint32_t m192_sector(void *ctx,uint32_t lbn,uint8_t out[512]){
 struct m192_env *e=ctx;e->reads++;
 if(e->fail_at&&e->reads==e->fail_at)return 0;
 for(unsigned i=0;i<512;i++)out[i]=m192_byte(lbn,i);
 return 1u;
}
static inline void m192_setup(struct m192_env *e){
 memset(e,0,sizeof(*e));e->chain.count=2;e->chain.blocks=6;
 e->chain.extents[0].lbn=40;e->chain.extents[0].blocks=3;
 e->chain.extents[1].lbn=80;e->chain.extents[1].blocks=3;
 assert(port_dq_ioc_init(&e->queue,m192_sector,e)==PORT_DQ_IOC_OK);
 assert(port_f11_chan_init(&e->table,&e->chain,&e->queue,3072)==PORT_F11_CHAN_OK);
}
static inline uint32_t m192_lbn(uint32_t offset){
 uint32_t b=offset/512u;return b<3u?40u+b:80u+b-3u;
}
static inline void m192_expected(uint8_t *out,uint32_t from,uint32_t count){
 for(uint32_t i=0;i<count;i++)out[i]=m192_byte(m192_lbn(from+i),(from+i)%512u);
}
#endif
