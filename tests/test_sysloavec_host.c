#include <assert.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include "vms_sysloavec.h"
static unsigned char image[256];
static void put16(unsigned i,uint16_t value){image[i]=(uint8_t)value;image[i+1]=(uint8_t)(value>>8);}
static void put32(unsigned i,uint32_t value){
    for(unsigned k=0;k<4;k++)image[i+k]=(uint8_t)(value>>(k*8u));
}
static void setup(void) {
    memset(image,0,sizeof(image));
    put32(0u,sizeof(image));put32(4u,0u);
    put16(8u,sizeof(image));image[10]=0x42u;image[11]=0;
    for(unsigned i=0;i<PORT_SYSLOA_COUNT;i++) {
        unsigned loc=PORT_SYSLOA_HEADER_BYTES+5u*i;
        image[loc]=(uint8_t)(i<7u?2u:i+1u==PORT_SYSLOA_COUNT?1u:3u);
        int32_t rel=(int32_t)(160u+i)-(int32_t)(loc+1u);
        put32(loc+1u,(uint32_t)rel);
    }
    put32(PORT_SYSLOA_HEADER_BYTES+5u*PORT_SYSLOA_COUNT,UINT32_MAX);
}
int main(void) {
    struct port_sysloa_table resident;
    port_sysloavec_init(&resident);
    assert(!port_sysloavec_address(&resident,PORT_SYSLOA_MCHK));
    assert(!port_sysloavec_address(&resident,PORT_SYSLOA_COUNT));
    struct port_sysloa_patch patch;
    memset(&patch,0xff,sizeof(patch));port_sysloavec_patch_init(&patch);
    assert(patch.count==15u && patch.words[14]==0u);
    setup();
    assert(port_sysloavec_link(&resident,image,sizeof(image),0x42u));
    for(unsigned i=0;i<PORT_SYSLOA_COUNT;i++)
        assert(port_sysloavec_address(&resident,i)==(uintptr_t)(image+160u+i));
    uintptr_t retained=port_sysloavec_address(&resident,PORT_SYSLOA_INIBOOTADP);
    assert(!port_sysloavec_link(&resident,image,sizeof(image),0x43u));
    assert(port_sysloavec_address(&resident,PORT_SYSLOA_INIBOOTADP)==retained);
    image[PORT_SYSLOA_HEADER_BYTES+5u*PORT_SYSLOA_COUNT]=0;
    assert(!port_sysloavec_link(&resident,image,sizeof(image),0x42u));
    assert(port_sysloavec_address(&resident,PORT_SYSLOA_INIBOOTADP)==retained);
    setup();image[PORT_SYSLOA_HEADER_BYTES+5u*8u]=2u;
    assert(!port_sysloavec_link(&resident,image,sizeof(image),0x42u));
    setup();put32(PORT_SYSLOA_HEADER_BYTES+5u*11u+1u,0x70000000u);
    assert(!port_sysloavec_link(&resident,image,sizeof(image),0x42u));
    setup();put32(0,31u);
    assert(!port_sysloavec_link(&resident,image,sizeof(image),0x42u));
    setup();put16(8,1u);
    assert(!port_sysloavec_link(&resident,image,sizeof(image),0x42u));
    setup();put32(4,1u);
    assert(!port_sysloavec_link(&resident,image,sizeof(image),0x42u));
    setup();
    assert(!port_sysloavec_link(&resident,image,sizeof(image)-1u,0x42u));
    puts("SYSLOAVEC loader vector and source-order validation PASS");
    return 0;
}
