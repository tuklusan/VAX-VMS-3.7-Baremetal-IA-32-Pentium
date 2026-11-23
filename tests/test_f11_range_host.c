#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include "vms_f11_range.h"
#include "vms_dq_imageprobe.h"
#define CHECK(x) do{if(!(x)){fprintf(stderr,"FIL$READVBN ranged test %u: %s\n",__LINE__,#x);return 1;}}while(0)
struct diskctx{FILE *disk;unsigned requests,fail_on;};
static uint32_t read_sector(void *p,uint32_t lbn,uint8_t out[512]){
    struct diskctx *d=p;d->requests++;
    if(d->fail_on&&d->requests==d->fail_on)return PORT_DQ_DIR_MAP;
    return fseek(d->disk,512L*(long)lbn,SEEK_SET)==0&&
           fread(out,1,512u,d->disk)==512u?PORT_DQ_DIR_OK:PORT_DQ_DIR_MAP;
}
int main(void){
    struct diskctx d={fopen("portdisk.img","rb"),0u,0u};CHECK(d.disk);
    uint8_t home[512],index[512],header[512];
    CHECK(read_sector(&d,1u,home)==PORT_DQ_DIR_OK);
    uint32_t lbn=0u;
    CHECK(port_dq_index_location(home,&lbn)==PORT_DQ_INDEX_OK);
    CHECK(read_sector(&d,lbn,index)==PORT_DQ_DIR_OK);
    struct port_dq_dir_fid fid={11u,11u,0u};
    CHECK(port_dq_image_header_lbn(home,index,&fid,&lbn)==PORT_DQ_IMAGE_OK);
    CHECK(read_sector(&d,lbn,header)==PORT_DQ_DIR_OK);
    CHECK(port_dq_image_verify_fh2(header,&fid)==PORT_DQ_IMAGE_OK);
    FILE *source=fopen("build/sys.exe","rb");CHECK(source);
    const uint32_t sizes[]={1u,32u,511u,512u,513u,1024u,2048u,4000u};
    for(unsigned i=0u;i<8u;i++)for(unsigned j=0u;j<8u;j++){
        uint32_t offset=4096u+37u*i+512u*j;
        uint32_t count=sizes[j];
        uint8_t output[4096],expected[4096];uint32_t complete=UINT32_MAX;
        memset(output,0xa5,sizeof output);
        d.requests=0;
        CHECK(port_f11_read_range(header,offset,count,output,sizeof output,
             read_sector,&d,&complete)==PORT_F11_RANGE_OK);
        CHECK(complete==count);
        CHECK(fseek(source,(long)offset,SEEK_SET)==0);
        CHECK(fread(expected,1,count,source)==count);
        CHECK(memcmp(output,expected,count)==0);
        CHECK(output[count]==0xa5);
        CHECK(d.requests==((offset%512u+count+511u)/512u));
    }
    uint8_t output[2048];memset(output,0x5a,sizeof output);
    uint32_t completed=0xdeadbeefu;
    d.requests=0;d.fail_on=2u;
    CHECK(port_f11_read_range(header,4096u+9u,1024u,output,sizeof output,
             read_sector,&d,&completed)==PORT_F11_RANGE_IO);
    CHECK(completed==503u&&d.requests==2u&&output[503u]==0x5a);
    d.fail_on=0u;
    completed=0xdeadbeefu;
    CHECK(port_f11_read_range(header,UINT32_MAX-1u,128u,output,sizeof output,
             read_sector,&d,&completed)==PORT_F11_RANGE_BAD);
    CHECK(completed==0xdeadbeefu);
    completed=0xdeadbeefu;
    CHECK(port_f11_read_range(header,4096u,2049u,output,sizeof output,
             read_sector,&d,&completed)==PORT_F11_RANGE_BAD);
    CHECK(completed==0xdeadbeefu);
    completed=0xdeadbeefu;
    CHECK(port_f11_read_range(header,0u,0u,output,sizeof output,
             read_sector,&d,&completed)==PORT_F11_RANGE_OK);
    CHECK(completed==0u);
    CHECK(port_f11_read_range(NULL,4096u,512u,output,sizeof output,
             read_sector,&d,&completed)==PORT_F11_RANGE_BAD);
    CHECK(port_f11_read_range(header,4096u,512u,output,sizeof output,
             NULL,&d,&completed)==PORT_F11_RANGE_BAD);
    CHECK(fclose(source)==0&&fclose(d.disk)==0);
    puts("Original FIL$READVBN byte-granular physically mapped read, partial I/O and fault containment PASS");
    return 0;
}
