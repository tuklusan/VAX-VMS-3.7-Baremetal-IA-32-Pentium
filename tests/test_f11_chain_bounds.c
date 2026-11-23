#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include "vms_f11_chain.h"
#define CHECK(x) do{if(!(x)){fprintf(stderr,"FH2 chain bound %d: %s\n",__LINE__,#x);return 1;}}while(0)
int main(void){
    struct port_f11_chain c={0};uint32_t answer=0xdeadbeefu;
    CHECK(port_f11_chain_lbn(NULL,1u,&answer)==PORT_F11_CHAIN_BAD);
    CHECK(port_f11_chain_lbn(&c,1u,&answer)==PORT_F11_CHAIN_BAD);
    CHECK(answer==0xdeadbeefu);
    c.count=1u;c.extents[0].blocks=1u;c.extents[0].lbn=100u;
    CHECK(port_f11_chain_lbn(&c,1u,&answer)==PORT_F11_CHAIN_OK&&answer==100u);
    answer=0xdeadbeefu;
    CHECK(port_f11_chain_lbn(&c,2u,&answer)==PORT_F11_CHAIN_MAP);
    CHECK(answer==0xdeadbeefu);
    CHECK(port_f11_chain_lbn(&c,0u,&answer)==PORT_F11_CHAIN_BAD);
    CHECK(port_f11_chain_lbn(&c,1u,NULL)==PORT_F11_CHAIN_BAD);
    c.count=65u;
    CHECK(port_f11_chain_lbn(&c,1u,&answer)==PORT_F11_CHAIN_BAD);
    c.count=1u;c.extents[0].lbn=UINT32_C(0x10000000);
    CHECK(port_f11_chain_lbn(&c,1u,&answer)==PORT_F11_CHAIN_MAP);
    c.extents[0].lbn=UINT32_C(0x0fffffff);c.extents[0].blocks=2u;
    CHECK(port_f11_chain_lbn(&c,1u,&answer)==PORT_F11_CHAIN_MAP);
    c.extents[0].blocks=0u;
    CHECK(port_f11_chain_lbn(&c,1u,&answer)==PORT_F11_CHAIN_MAP);
    c.extents[0].lbn=100u;c.extents[0].blocks=UINT32_MAX;
    CHECK(port_f11_chain_lbn(&c,UINT32_MAX,&answer)==PORT_F11_CHAIN_MAP);
    memset(&c,0,sizeof c);
    c.count=PORT_F11_CHAIN_MAX_EXTENTS;
    for(unsigned i=0;i<PORT_F11_CHAIN_MAX_EXTENTS;i++){
        c.extents[i].lbn=100u+2u*i;c.extents[i].blocks=2u;
    }
    for(uint32_t v=1;v<=PORT_F11_CHAIN_MAX_EXTENTS*2u;v++){
        CHECK(port_f11_chain_lbn(&c,v,&answer)==PORT_F11_CHAIN_OK);
        CHECK(answer==100u+v-1u);
    }
    answer=0xcafebabeu;
    CHECK(port_f11_chain_lbn(&c,129u,&answer)==PORT_F11_CHAIN_MAP);
    CHECK(answer==0xcafebabeu);
    puts("FIL$RDCHKFILHDR 64-entry normalized extent boundaries, missing/overflow VBNs PASS");
    return 0;
}
