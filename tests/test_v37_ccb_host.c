#include "vms_v37_ccb.h"
#include <stdio.h>

#define CHECK(x) do{if(!(x)){fprintf(stderr,"CCB check failed line %d\n",__LINE__);return 1;}}while(0)
static void w16(uint8_t *p,uint16_t v){p[0]=(uint8_t)v;p[1]=(uint8_t)(v>>8);}
static void w32(uint8_t *p,uint32_t v){for(unsigned i=0;i<4;i++)p[i]=(uint8_t)(v>>(i*8u));}
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
    puts("SYSQIOREQ VCHAN backward CCB lookup, pending window and access-mode checks PASS");
    return 0;
}

