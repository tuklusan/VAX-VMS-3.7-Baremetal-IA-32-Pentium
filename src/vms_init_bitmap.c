#include "vms_init_bitmap.h"
#include "vms_f11_home.h"
static void set16(uint8_t *p,uint16_t n){p[0]=(uint8_t)n;p[1]=(uint8_t)(n>>8);}
static void set32(uint8_t *p,uint32_t n){set16(p,(uint16_t)n);set16(p+2u,(uint16_t)(n>>16));}
static void clear(uint8_t b[512],uint8_t c){
    for(unsigned i=0;i<512u;i++)b[i]=c;
}
uint32_t port_init_bitmap(const struct port_init_bitmap_options *o,
                          const struct port_init_allocation *alloc,
                          size_t count,uint8_t b[512],
                          port_init_write_block write,void *ctx) {
    if(!o||!b||!write||!o->bitmap_blocks||!o->cluster||
       (count&&!alloc)||o->volume_blocks==0u)return PORT_INIT_MFD_BAD_ARGUMENT;
    clear(b,0);
    if(o->structure1){
        uint32_t blocks=o->bitmap_blocks-1u;
        if(blocks>126u)blocks=0u;
        b[3]=(uint8_t)blocks;
        for(uint32_t i=0;i<blocks;i++)set32(b+4u+4u*i,4096u);
        set32(b+4u+4u*blocks,(o->volume_blocks<<16u)|
                                  (o->volume_blocks>>16u));
    }else{
        set16(b,513u);                     /* SCB$C_LEVEL2 + 1 */
        set16(b+2u,(uint16_t)o->cluster);
        set32(b+4u,o->volume_blocks);
        uint64_t physical=(uint64_t)o->sectors_per_track*
                           o->tracks_per_cylinder*o->cylinders;
        set32(b+8u,(uint32_t)(physical/o->volume_blocks));
        set32(b+12u,o->sectors_per_track);
        set32(b+16u,o->tracks_per_cylinder);
        set32(b+20u,o->cylinders);
        port_f11_checksum2(b,510u);
    }
    uint32_t rc=write(ctx,o->bitmap_lbn,b);
    if(!(rc&1u))return rc;
    /* The allocation table records used clusters. Each bit is 1 for free
     * storage and 0 for allocated storage, as in INIT_BITMAP. */
    clear(b,0xff);
    uint32_t map_lbn=o->bitmap_lbn+1u;
    uint32_t map_end=o->bitmap_lbn+o->bitmap_blocks;
    if(map_end<=map_lbn)return PORT_INIT_MFD_BAD_ARGUMENT;
    uint32_t last=0u;
    for(size_t processed=0;processed<count;processed++){
        size_t selected=count;
        uint32_t next=UINT32_MAX;
        for(size_t j=0;j<count;j++){
            if(!alloc[j].block_count)continue;
            if(alloc[j].start_lbn>=last && alloc[j].start_lbn<next){
                selected=j;next=alloc[j].start_lbn;
            }
        }
        if(selected==count)break;
        if(next==UINT32_MAX || alloc[selected].block_count>UINT32_MAX-next)
            return PORT_INIT_MFD_BAD_ARGUMENT;
        last=next+1u;
        uint32_t bitcount=alloc[selected].block_count/o->cluster;
        uint32_t first=next/o->cluster;
        uint32_t bitpos=first-(map_lbn-o->bitmap_lbn-1u)*4096u;
        while(bitpos>=4096u){
            if(map_lbn>=map_end)return PORT_INIT_MFD_BAD_ARGUMENT;
            rc=write(ctx,map_lbn++,b);
            if(!(rc&1u))return rc;
            clear(b,0xff);
            bitpos-=4096u;
        }
        while(bitcount--){
            if(map_lbn>=map_end)return PORT_INIT_MFD_BAD_ARGUMENT;
            b[bitpos>>3u] &= (uint8_t)~(1u<<(bitpos&7u));
            if(++bitpos==4096u){
                rc=write(ctx,map_lbn++,b);
                if(!(rc&1u))return rc;
                clear(b,0xff);
                bitpos=0;
            }
        }
    }
    if(map_lbn>=map_end)return PORT_INIT_MFD_BAD_ARGUMENT;
    rc=write(ctx,map_lbn++,b);
    if(!(rc&1u))return rc;
    clear(b,0);
    while(map_lbn<map_end){
        rc=write(ctx,map_lbn++,b);
        if(!(rc&1u))return rc;
    }
    return PORT_INIT_MFD_NORMAL;
}
