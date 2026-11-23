#ifndef DDTRELOC_TEST_COMMON_H
#define DDTRELOC_TEST_COMMON_H
#include "vms_ddtreloc.h"
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#define REQUIRE(x) do {if(!(x)) {fprintf(stderr,"DDT case %s:%d %s\n",__FILE__,__LINE__,#x);return 1;}}while(0)
static inline void ddtreloc_fixture(uint8_t *b,size_t len){
    memset(b,0,len);
    const unsigned offsets[]={0u,4u,8u,12u,16u,24u,28u,32u};
    for(unsigned i=0;i<8u;i++)port_ddtreloc_write32(b+offsets[i],0x110u+4u*i);
    port_ddtreloc_write32(b+8u,40u);
    b[36u]=136u;
    for(unsigned i=0;i<10u&&64u+i*12u+4u<=len;i++)
        port_ddtreloc_write32(b+64u+i*12u,0x120u+i);
}
#endif
