#include "vms_v37_ccb.h"
#include <stdio.h>
#include <string.h>

#define CHECK(x) do{if(!(x)){fprintf(stderr,"CCB check failed line %d\n",__LINE__);return 1;}}while(0)
static void w16(uint8_t *p,uint16_t v){p[0]=(uint8_t)v;p[1]=(uint8_t)(v>>8);}
static void w32(uint8_t *p,uint32_t v){for(unsigned i=0;i<4;i++)p[i]=(uint8_t)(v>>(i*8u));}
static uint16_t r16(const uint8_t *p){return (uint16_t)(p[0]|((uint16_t)p[1]<<8));}
static uint32_t r32(const uint8_t *p){return (uint32_t)p[0]|((uint32_t)p[1]<<8)|((uint32_t)p[2]<<16)|((uint32_t)p[3]<<24);}
struct assign_fixture {uint32_t lock_status,owner_ok,access_status,locks,unlocks,checks;};
static uint32_t assign_lock(void *v){struct assign_fixture *f=v;f->locks++;return f->lock_status;}
static void assign_unlock(void *v){((struct assign_fixture *)v)->unlocks++;}
static uint32_t assign_owner(void *v,uint32_t owner,uint32_t pid){
    struct assign_fixture *f=v;return f->owner_ok||owner==pid;
}
static uint32_t assign_access(void *v,uint32_t arb,const uint8_t *ucb,size_t n){
    struct assign_fixture *f=v;(void)ucb;(void)n;f->checks++;return f->access_status&&arb;
}
int main(void){
    _Alignas(4) uint8_t storage[64]={0};
    uint8_t *base=storage+48u;
    uint8_t *ccb=base-32u;
    w32(ccb+PORT_V37_CCB_UCB,0x12345678u);
    w32(ccb+PORT_V37_CCB_WIND,0x1001u);
    ccb[PORT_V37_CCB_STS]=0x5au;
    ccb[PORT_V37_CCB_AMOD]=4u;
    w16(ccb+PORT_V37_CCB_IOC,3u);
    struct port_v37_ccb_view v={0};
    CHECK(port_v37_ccb_resolve(base,48u,32u,48u,3u,&v)==PORT_V37_CCB_OK);
    CHECK(v.ccb==ccb&&v.ucb==0x12345678u&&v.window==0x1001u&&
          v.pending&&v.outstanding==3u&&v.status==0x5au&&
          v.access_limit==4u);
    CHECK(port_v37_ccb_resolve(base,48u,32u,48u,4u,&v)==PORT_V37_CCB_BAD);
    CHECK(port_v37_ccb_resolve(base,48u,32u,48u,2u,&v)==PORT_V37_CCB_OK);
    CHECK(port_v37_ccb_resolve(base,48u,0u,48u,3u,&v)==PORT_V37_CCB_IVCHAN);
    CHECK(port_v37_ccb_resolve(base,48u,33u,48u,3u,&v)==PORT_V37_CCB_IVCHAN);
    CHECK(port_v37_ccb_resolve(base,48u,UINT32_C(0x10020),48u,3u,&v)==PORT_V37_CCB_IVCHAN);
    CHECK(port_v37_ccb_resolve(base,48u,64u,48u,3u,&v)==PORT_V37_CCB_IVCHAN);
    w32(ccb+PORT_V37_CCB_UCB,0u);
    CHECK(port_v37_ccb_resolve(base,48u,32u,48u,3u,&v)==PORT_V37_CCB_IVCHAN);
    uint16_t high_water=16u,channel=0u;uint8_t *found=0;
    memset(storage,0,sizeof storage);
    storage[3u*PORT_V37_CCB_BYTES+PORT_V37_CCB_AMOD]=1u;
    storage[2u*PORT_V37_CCB_BYTES+PORT_V37_CCB_AMOD]=4u;
    CHECK(port_v37_ccb_find_free(storage+64u,64u,3u,&high_water,
         &channel,&found)==PORT_V37_CCB_OK&&channel==48u&&
         found==storage+16u&&high_water==48u);
    found=0;channel=0u;storage[PORT_V37_CCB_BYTES+
         PORT_V37_CCB_AMOD]=2u;
    CHECK(port_v37_ccb_find_free(storage+64u,64u,3u,&high_water,
         &channel,&found)==PORT_V37_CCB_NOCHANNEL&&found==0&&channel==0u);
    CHECK(port_v37_ccb_find_free(storage+64u,32u,3u,&high_water,
         &channel,&found)==PORT_V37_CCB_BAD);
    CHECK(port_v37_ccb_find_free(storage+64u,64u,0u,&high_water,
         &channel,&found)==PORT_V37_CCB_NOCHANNEL);

    _Alignas(4) uint8_t ccb_store[64]={0},ucb_store[PORT_V37_UCB_FIXED_BYTES]={0};
    uint8_t *ccb_base=ccb_store+32u;
    w16(ucb_store+8u,PORT_V37_UCB_FIXED_BYTES);
    w16(ucb_store+PORT_V37_UCB_REFC,2u);
    struct assign_fixture af={.lock_status=1u,.access_status=1u};
    const struct port_v37_assign_ops aops={&af,assign_lock,assign_unlock,
                                           assign_owner,assign_access};
    uint16_t assigned=0u,awater=0u;
    CHECK(port_v37_ccb_assign_local(ccb_base,32u,2u,&awater,ucb_store,
          sizeof ucb_store,0x12340000u,0x11112222u,0x2000u,2u,0u,
          &aops,&assigned)==PORT_V37_CCB_OK);
    CHECK(assigned==16u&&awater==16u&&
          r32(ccb_base-16u)==0x12340000u&&
          (ccb_base-16u)[PORT_V37_CCB_AMOD]==3u&&
          r16(ucb_store+PORT_V37_UCB_REFC)==3u&&
          r32(ucb_store+PORT_V37_UCB_PID)==0x11112222u&&
          af.locks==1u&&af.unlocks==1u&&af.checks==1u);

    memset(ccb_store,0,sizeof ccb_store);memset(ucb_store,0,sizeof ucb_store);
    w16(ucb_store+8u,PORT_V37_UCB_FIXED_BYTES);
    w16(ucb_store+PORT_V37_UCB_REFC,1u);
    w32(ucb_store+PORT_V37_UCB_PID,0x33334444u);
    awater=0u;assigned=0xbeefu;af=(struct assign_fixture){.lock_status=1u};
    CHECK(port_v37_ccb_assign_local(ccb_base,32u,2u,&awater,ucb_store,
          sizeof ucb_store,0x12340000u,0x11112222u,0x2000u,2u,0u,
          &aops,&assigned)==PORT_V37_ASSIGN_DEVALLOC);
    CHECK(assigned==0xbeefu&&r16(ucb_store+PORT_V37_UCB_REFC)==1u&&
          !(ccb_base-16u)[PORT_V37_CCB_AMOD]&&af.unlocks==1u);

    memset(ccb_store,0,sizeof ccb_store);memset(ucb_store,0,sizeof ucb_store);
    w16(ucb_store+8u,PORT_V37_UCB_FIXED_BYTES);
    w32(ucb_store+PORT_V37_UCB_DEVCHAR,PORT_V37_UCB_DEV_SHR);
    af=(struct assign_fixture){.lock_status=1u};awater=0u;
    CHECK(port_v37_ccb_assign_local(ccb_base,32u,2u,&awater,ucb_store,
          sizeof ucb_store,0x12340000u,0x11112222u,0x2000u,1u,0u,
          &aops,&assigned)==PORT_V37_CCB_OK);
    CHECK(!af.checks&&!r32(ucb_store+PORT_V37_UCB_PID)&&
          r16(ucb_store+PORT_V37_UCB_REFC)==1u);

    memset(ccb_store,0,sizeof ccb_store);memset(ucb_store,0,sizeof ucb_store);
    w16(ucb_store+8u,PORT_V37_UCB_FIXED_BYTES);
    af=(struct assign_fixture){.lock_status=1u,.access_status=0u};awater=0u;
    CHECK(port_v37_ccb_assign_local(ccb_base,32u,2u,&awater,ucb_store,
          sizeof ucb_store,0x12340000u,0x11112222u,0x2000u,1u,0u,
          &aops,&assigned)==PORT_V37_ASSIGN_NOPRIV);
    CHECK(!r32(ucb_store+PORT_V37_UCB_PID)&&
          !(ccb_base-16u)[PORT_V37_CCB_AMOD]&&af.unlocks==1u);

    memset(ccb_store,0,sizeof ccb_store);memset(ucb_store,0,sizeof ucb_store);
    w16(ucb_store+8u,PORT_V37_UCB_FIXED_BYTES);
    w16(ucb_store+PORT_V37_UCB_REFC,UINT16_MAX);
    af=(struct assign_fixture){.lock_status=1u,.access_status=1u};awater=0u;
    CHECK(port_v37_ccb_assign_local(ccb_base,32u,2u,&awater,ucb_store,
          sizeof ucb_store,0x12340000u,0x11112222u,0x2000u,1u,0u,
          &aops,&assigned)==PORT_V37_CCB_OK);
    CHECK(r32(ucb_store+PORT_V37_UCB_PID)==0x11112222u&&
          (ccb_base-16u)[PORT_V37_CCB_AMOD]==2u&&
          r16(ucb_store+PORT_V37_UCB_REFC)==0u&&af.unlocks==1u);

    CHECK(port_v37_ccb_assign_local(ccb_base,32u,2u,&awater,ucb_store,
          sizeof ucb_store,0x12340000u,0x11112222u,0x2000u,1u,0x99u,
          &aops,&assigned)==PORT_V37_ASSIGN_UNSUPPORTED);
    puts("SYSASSIGN local UCB ownership/refcounts, IOC$FFCHAN and VCHAN checks PASS");
    return 0;
}
