/* Verify that SYSLOA386's *actual* ELF bytes contain a source-ordered,
 * self-relative 23-entry SYSLOAVEC list resolving to the loaded image. */
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include "vms_sysloavec.h"
#include "vms_sysloa386.h"
#define CHECK(x) do{if(!(x)){fprintf(stderr,"FAIL %s:%d %s\n",__FILE__,__LINE__,#x);return 1;}}while(0)
static uint16_t get16(const uint8_t *b){return (uint16_t)(b[0]|(b[1]<<8));}
static uint32_t get32(const uint8_t *b){
    return (uint32_t)get16(b)|((uint32_t)get16(b+2)<<16);
}
static uint8_t loaded[65536u];
int main(int argc,char **argv){
    CHECK(argc==2);
    FILE *f=fopen(argv[1],"rb");CHECK(f);
    uint8_t h[512];CHECK(fread(h,1,512u,f)==512u);
    CHECK(!memcmp(h,"\177ELF",4u)&&get16(h+18u)==3u);
    CHECK(get32(h+24u)>=0x3e0000u&&get32(h+24u)<0x3f0000u);
    const uint32_t phoff=get32(h+28u);
    const uint16_t count=get16(h+44u);
    CHECK(phoff>=52u&&phoff+(uint32_t)count*32u<=512u);
    for(unsigned i=0;i<count;i++){
        const uint8_t *p=h+phoff+i*32u;
        if(get32(p)!=1u)continue;
        const uint32_t off=get32(p+4u),at=get32(p+12u);
        const uint32_t filesz=get32(p+16u),memsz=get32(p+20u);
        CHECK(at>=0x3e0000u&&at<0x3f0000u&&
              memsz<=0x3f0000u-at&&filesz<=memsz);
        CHECK(!fseek(f,(long)off,SEEK_SET));
        CHECK(fread(loaded+at-0x3e0000u,1,filesz,f)==filesz);
    }
    fclose(f);
    const uint32_t extent=get32(loaded);
    CHECK(extent>=132u&&extent<=65535u&&get16(loaded+8u)==extent);
    struct port_sysloa_table table;
    port_sysloavec_init(&table);
    CHECK(port_sysloavec_link(&table,loaded,extent,
                              PORT_SYSLOA386_DYN_LOADCODE));
    uintptr_t boot=port_sysloavec_address(&table,PORT_SYSLOA_INIBOOTADP);
    uintptr_t cpu=port_sysloavec_address(&table,PORT_SYSLOA_INIPROCREG);
    uintptr_t exception=port_sysloavec_address(&table,PORT_SYSLOA_MCHK);
    uintptr_t counters=port_sysloavec_address(&table,PORT_SYSLOA_MCHK_ERRCNT);
    CHECK(boot>=(uintptr_t)loaded&&boot<(uintptr_t)(loaded+extent));
    CHECK(cpu>=(uintptr_t)loaded&&cpu<(uintptr_t)(loaded+extent));
    CHECK(exception>=(uintptr_t)loaded&&exception<(uintptr_t)(loaded+extent));
    CHECK(counters>=(uintptr_t)loaded&&counters<(uintptr_t)(loaded+extent));
    CHECK(boot!=cpu && boot!=exception && cpu!=exception);
    CHECK(port_sysloavec_address(&table,PORT_SYSLOA_CI_INITIAL)==exception);
    CHECK(port_sysloavec_address(&table,PORT_SYSLOA_UBA_INITIAL)==exception);
    CHECK(get32(loaded+12u+5u*PORT_SYSLOA_COUNT)==UINT32_MAX);
    loaded[12u+5u*PORT_SYSLOA_INIPROCREG]=0u;
    port_sysloavec_init(&table);
    CHECK(!port_sysloavec_link(&table,loaded,extent,
                               PORT_SYSLOA386_DYN_LOADCODE));
    CHECK(!table.loaded);
    puts("SYSLOA386 ELF32: 23 typed vectors, 2 real CPU/disk services, fail-closed others PASS");
    return 0;
}
