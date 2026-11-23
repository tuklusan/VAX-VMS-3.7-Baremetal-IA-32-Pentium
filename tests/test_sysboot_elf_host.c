#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "vms_sysboot_elf.h"
#include "vms_boot_files11.h"
#define CHECK(x) do{if(!(x)){fprintf(stderr,"FAIL %s:%d: %s\n",__FILE__,__LINE__,#x);return 1;}}while(0)
struct test_disk { FILE *f; int corrupt_header; int fail_sector; };
static uint16_t rd16(const uint8_t *p){return (uint16_t)(p[0]|((uint16_t)p[1]<<8));}
static uint32_t rd32(const uint8_t *p){return (uint32_t)p[0]|((uint32_t)p[1]<<8)|((uint32_t)p[2]<<16)|((uint32_t)p[3]<<24);}
static uint32_t transfer(void *ctx,uint32_t mode,uint32_t fn,uint32_t lbn,uint32_t size,void *ptr){
    struct test_disk *d=ctx;
    if(mode||fn!=1u||size!=512u||!ptr||
       (d->fail_sector && lbn==33u)||
       fseek(d->f,(long)lbn*512,SEEK_SET)||fread(ptr,1,512,d->f)!=512u)
        return BOO_IO_FAILED;
    if(d->corrupt_header && lbn==33u)((uint8_t *)ptr)[0]=0u;
    return BOO_IO_NORMAL;
}
static uint8_t resident[PORT_SYSLOA386_ELF_LIMIT-PORT_SYS_ELF_START];
struct writes { uint32_t calls,bytes,zeroes; };
static int write_ram(void *ctx,uint32_t at,const uint8_t *bytes,uint32_t len){
    struct writes *w=ctx;
    if(at<PORT_SYS_ELF_START||at>PORT_SYSLOA386_ELF_LIMIT||
       len>PORT_SYSLOA386_ELF_LIMIT-at)return 0;
    memcpy(resident+(at-PORT_SYS_ELF_START),bytes,len);
    w->calls++;w->bytes+=len;
    return 1;
}
static int clear_ram(void *ctx,uint32_t at,uint32_t len){
    struct writes *w=ctx;
    if(at<PORT_SYS_ELF_START||at>PORT_SYSLOA386_ELF_LIMIT||
       len>PORT_SYSLOA386_ELF_LIMIT-at)return 0;
    memset(resident+(at-PORT_SYS_ELF_START),0,len);
    w->zeroes+=len;
    return 1;
}
int main(int argc,char **argv){
    CHECK(argc==5);
    struct test_disk d={fopen(argv[1],"rb"),0,0};CHECK(d.f);
    FILE *elf=fopen(argv[2],"rb");CHECK(elf);
    const struct boo_io_driver driver={transfer,&d,0};
    struct port_f11_volume volume;
    CHECK(port_f11_mount(&driver,1u,&volume)==PORT_F11_HOME_OK);
    struct port_f11_fid fid;
    uint8_t header[512];uint32_t first=0,blocks=0;
    CHECK(port_f11_open_named_file(&driver,1u,&volume,"[SYSEXE]SYS.EXE",
            &fid,header,&first,&blocks)==PORT_F11_HOME_OK);
    CHECK(fid.number==11u&&fid.sequence==11u&&first==33u&&blocks>1u);
    struct writes w={0};
    const struct port_sys_elf_memory memory={&w,write_ram,clear_ram};
    memset(resident,0xa5,sizeof resident);
    uint32_t entry=0u;
    CHECK(port_sysboot_load_elf(&driver,1u,header,blocks,&memory,&entry)==PORT_SYS_ELF_OK);
    CHECK(entry>=PORT_SYS_ELF_START && entry<PORT_SYS_ELF_LIMIT);
    uint8_t hdr[512];CHECK(fread(hdr,1,512,elf)==512u);
    CHECK(rd16(hdr+18u)==3u&&entry==rd32(hdr+24u));
    uint32_t total=0,zeros=0;
    for(unsigned i=0;i<rd16(hdr+44u);i++){
        const uint8_t *ph=hdr+rd32(hdr+28u)+i*32u;
        if(rd32(ph)!=1u || rd32(ph+20u)==0u)continue;
        uint32_t off=rd32(ph+4u),at=rd32(ph+12u),filesz=rd32(ph+16u),size=rd32(ph+20u);
        CHECK(at>=PORT_SYS_ELF_START && at<PORT_SYS_ELF_LIMIT);
        CHECK(!fseek(elf,(long)off,SEEK_SET));
        uint8_t *expected=malloc(filesz?filesz:1u);
        CHECK(expected);
        CHECK(fread(expected,1,filesz,elf)==filesz);
        CHECK(memcmp(resident+at-PORT_SYS_ELF_START,expected,filesz)==0);
        for(uint32_t j=filesz;j<size;j++)
            CHECK(resident[at-PORT_SYS_ELF_START+j]==0u);
        total+=filesz;zeros+=size-filesz;
        free(expected);
    }
    CHECK(w.bytes==total&&w.zeroes==zeros&&w.calls>1u);
    /* Reject invalid executable metadata before touching any RAM. */
    memset(resident,0xa5,sizeof resident);w.calls=w.bytes=w.zeroes=0u;
    d.corrupt_header=1;
    CHECK(port_sysboot_load_elf(&driver,1u,header,blocks,&memory,&entry)==
          PORT_SYS_ELF_BAD_HEADER);
    CHECK(!w.calls&&!w.bytes&&!w.zeroes);
    for(unsigned i=0;i<sizeof resident;i++)CHECK(resident[i]==0xa5u);
    d.corrupt_header=0;d.fail_sector=1;
    CHECK(port_sysboot_load_elf(&driver,1u,header,blocks,&memory,&entry)==
          PORT_SYS_ELF_DISK_ERROR);
    CHECK(!w.calls);
    CHECK(port_sysboot_load_elf(&driver,1u,header,0u,&memory,&entry)==
          PORT_SYS_ELF_BAD_ARG);
    /* Independently linked INILOA must be found through its actual new
     * INDEXF extension, and its executable text must load only into the
     * reserved second aperture. The SYS.EXE area must stay untouched. */
    d.fail_sector=0;d.corrupt_header=0;
    FILE *initelf=fopen(argv[3],"rb");CHECK(initelf);
    CHECK(port_f11_open_named_file(&driver,1u,&volume,"[SYSEXE]INILOA.EXE",
          &fid,header,&first,&blocks)==PORT_F11_HOME_OK);
    CHECK(fid.number==12u&&fid.sequence==12u&&first>=114u&&blocks>=2u);
    memset(resident,0xa5,sizeof resident);
    w.calls=w.bytes=w.zeroes=0u;
    uint32_t inientry=0u;
    CHECK(port_sysboot_load_elf_range(&driver,1u,header,blocks,&memory,
          PORT_INILOA_ELF_START,PORT_INILOA_ELF_LIMIT,&inientry)==PORT_SYS_ELF_OK);
    CHECK(inientry>=PORT_INILOA_ELF_START&&inientry<PORT_INILOA_ELF_LIMIT);
    CHECK(fread(hdr,1,512,initelf)==512u&&inientry==rd32(hdr+24u));
    uint32_t initbytes=0,initzeros=0;
    for(unsigned i=0;i<rd16(hdr+44u);i++){
        const uint8_t *ph=hdr+rd32(hdr+28u)+i*32u;
        if(rd32(ph)!=1u || rd32(ph+20u)==0u)continue;
        uint32_t off=rd32(ph+4u),at=rd32(ph+12u);
        uint32_t bytes=rd32(ph+16u),memsz=rd32(ph+20u);
        CHECK(at>=PORT_INILOA_ELF_START && at+memsz<=PORT_INILOA_ELF_LIMIT);
        CHECK(!fseek(initelf,(long)off,SEEK_SET));
        uint8_t *expected=malloc(bytes?bytes:1u);CHECK(expected);
        CHECK(fread(expected,1,bytes,initelf)==bytes);
        CHECK(!memcmp(resident+at-PORT_SYS_ELF_START,expected,bytes));
        for(uint32_t j=bytes;j<memsz;j++)
            CHECK(resident[at-PORT_SYS_ELF_START+j]==0u);
        initbytes+=bytes;initzeros+=memsz-bytes;free(expected);
    }
    CHECK(w.bytes==initbytes&&w.zeroes==initzeros&&w.calls>0u);
    for(uint32_t j=0;j<PORT_INILOA_ELF_START-PORT_SYS_ELF_START;j++)
        CHECK(resident[j]==0xa5u);
    /* Requiring the SYS range must reject the image before any writes. */
    w.calls=w.bytes=w.zeroes=0u;
    CHECK(port_sysboot_load_elf(&driver,1u,header,blocks,&memory,&inientry)==
          PORT_SYS_ELF_BAD_SEGMENT);
    CHECK(!w.calls);
    fclose(initelf);
    /* Third, CPU-dependent loadable image follows SYSLOAVEC source order.
     * The FID13 INDEXF extension is real; all three images share an active
     * Files-11 disk but occupy disjoint supervisor apertures. */
    FILE *sysloa=fopen(argv[4],"rb");CHECK(sysloa);
    CHECK(port_f11_open_named_file(&driver,1u,&volume,
          "[SYSEXE]SYSLOA386.EXE",&fid,header,&first,&blocks)==PORT_F11_HOME_OK);
    CHECK(fid.number==13u&&fid.sequence==13u&&first>114u&&blocks>1u);
    memset(resident,0xa5,sizeof resident);
    w.calls=w.bytes=w.zeroes=0u;
    uint32_t cpuentry=0;
    CHECK(port_sysboot_load_elf_range(&driver,1u,header,blocks,&memory,
          PORT_SYSLOA386_ELF_START,PORT_SYSLOA386_ELF_LIMIT,
          &cpuentry)==PORT_SYS_ELF_OK);
    CHECK(cpuentry>=PORT_SYSLOA386_ELF_START&&cpuentry<PORT_SYSLOA386_ELF_LIMIT);
    CHECK(fread(hdr,1,512,sysloa)==512u&&cpuentry==rd32(hdr+24u));
    uint32_t cpu_total=0,cpu_zero=0;
    for(unsigned i=0;i<rd16(hdr+44u);i++){
        const uint8_t *ph=hdr+rd32(hdr+28u)+i*32u;
        if(rd32(ph)!=1u||rd32(ph+20u)==0u)continue;
        uint32_t off=rd32(ph+4u),at=rd32(ph+12u);
        uint32_t bytes=rd32(ph+16u),memsz=rd32(ph+20u);
        CHECK(at>=PORT_SYSLOA386_ELF_START && at+memsz<=PORT_SYSLOA386_ELF_LIMIT);
        CHECK(!fseek(sysloa,(long)off,SEEK_SET));
        uint8_t *expected=malloc(bytes?bytes:1u);CHECK(expected);
        CHECK(fread(expected,1,bytes,sysloa)==bytes);
        CHECK(!memcmp(resident+at-PORT_SYS_ELF_START,expected,bytes));
        for(uint32_t j=bytes;j<memsz;j++)
            CHECK(resident[at-PORT_SYS_ELF_START+j]==0u);
        cpu_total+=bytes;cpu_zero+=memsz-bytes;free(expected);
    }
    CHECK(w.bytes==cpu_total&&w.zeroes==cpu_zero&&w.calls>0u);
    for(uint32_t j=0;j<PORT_SYSLOA386_ELF_START-PORT_SYS_ELF_START;j++)
        CHECK(resident[j]==0xa5u);
    w.calls=w.bytes=w.zeroes=0u;
    CHECK(port_sysboot_load_elf_range(&driver,1u,header,blocks,&memory,
          PORT_INILOA_ELF_START,PORT_INILOA_ELF_LIMIT,
          &cpuentry)==PORT_SYS_ELF_BAD_SEGMENT);
    CHECK(!w.calls);
    fclose(sysloa);
    fclose(elf);fclose(d.f);
    puts("SYSBOOT SYS.EXE, SYSLOA386.EXE and INILOA.EXE real Files-11/ELF32 load PASS");
    return 0;
}
