#include "vms_init_index.h"
#include "vms_f11_home.h"

static void set16(uint8_t *p,uint16_t n){p[0]=(uint8_t)n;p[1]=(uint8_t)(n>>8);}
static void set32(uint8_t *p,uint32_t n){set16(p,(uint16_t)n);set16(p+2u,(uint16_t)(n>>16));}
static uint16_t get16(const uint8_t *p){return (uint16_t)(p[0]|((uint16_t)p[1]<<8));}
static uint32_t get32(const uint8_t *p){return (uint32_t)p[0]|((uint32_t)p[1]<<8)|
    ((uint32_t)p[2]<<16)|((uint32_t)p[3]<<24);}
static void fill(uint8_t *p,unsigned n,uint8_t v){for(unsigned i=0;i<n;i++)p[i]=v;}
static void write_text(uint8_t *p,unsigned n,const char *text){
    int ended=text?0:1;
    for(unsigned i=0;i<n;i++){
        if(!ended && !text[i])ended=1;
        p[i]=ended?(uint8_t)' ':(uint8_t)text[i];
    }
}
static uint32_t save(port_init_write_block cb,void *ctx,uint32_t lbn,
                     const uint8_t b[512]) {
    return cb(ctx,lbn,b);
}
static uint32_t home_block(uint8_t b[512],port_init_write_block cb,
                            void *ctx) {
    port_f11_checksum2(b,58);
    port_f11_checksum2(b,510);
    uint32_t lbn=get32(b);
    uint32_t rc=save(cb,ctx,lbn,b);
    if(!(rc&1u))return rc;
    set32(b,lbn+1u);
    set16(b+16u,(uint16_t)(get16(b+16u)+1u));
    return PORT_INIT_MFD_NORMAL;
}
static uint32_t make_pointer(uint8_t b[512],uint32_t count,uint32_t lbn) {
    unsigned used=b[58u];
    unsigned at=130u+used*2u;
    if(!count||at>=510u)return PORT_INIT_MFD_BAD_ARGUMENT;
    if(count<=256u && lbn<(1u<<22u)) {
        if(at+4u>510u)return PORT_INIT_MFD_BAD_ARGUMENT;
        set16(b+at,(uint16_t)(0x4000u | ((lbn>>16u)&63u)<<8u |
                               (count-1u)));
        set16(b+at+2u,(uint16_t)lbn);
        b[58u]=(uint8_t)(used+2u);
    }else if(count<=16384u) {
        if(at+6u>510u)return PORT_INIT_MFD_BAD_ARGUMENT;
        set16(b+at,(uint16_t)(0x8000u|(count-1u)));
        set32(b+at+2u,lbn);
        b[58u]=(uint8_t)(used+3u);
    }else if(count<=(1u<<30u)) {
        if(at+8u>510u)return PORT_INIT_MFD_BAD_ARGUMENT;
        set16(b+at,(uint16_t)(0xc000u|((count-1u)>>16u)));
        set16(b+at+2u,(uint16_t)(count-1u));
        set32(b+at+4u,lbn);
        b[58u]=(uint8_t)(used+4u);
    }else return PORT_INIT_MFD_BAD_ARGUMENT;
    return PORT_INIT_MFD_NORMAL;
}
static void header_checksum(uint8_t b[512]){
    port_f11_checksum2(b,510u);
}
static void set_file(uint8_t b[512],uint16_t fid,const char *name){
    set16(b+8u,fid);set16(b+10u,fid);
    int ended=name?0:1;
    for(unsigned i=0;i<20u;i++){
        if(!ended&&!name[i])ended=1;
        b[76u+i]=ended?(uint8_t)' ':(uint8_t)name[i];
    }
}
static uint32_t write_header(port_init_write_block cb,void *ctx,
                              uint8_t b[512],uint32_t lbn){
    header_checksum(b);return save(cb,ctx,lbn,b);
}
/* PC version of INIT_INDEX's PDP-11 non-system boot warning. The IDE
 * volume is not a bootable VMS system until the true PC bootstrap exists. */
static void make_non_system_bootblock(uint8_t b[512],const char *label){
    fill(b,512,0);
    /* BIOS real-mode loader: print the volume label and stop. */
    const uint8_t x86[]={
        0xfa,                   /* CLI */
        0x31,0xc0,              /* XOR AX,AX */
        0x8e,0xd8,              /* MOV DS,AX */
        0xbe,0x1a,0x7c,         /* MOV SI,7c1a (message) */
        0xb4,0x0e,              /* MOV AH,0e */
        0xac,                   /* LODSB */
        0x84,0xc0,              /* TEST AL,AL */
        0x74,0x04,              /* JZ halt */
        0xcd,0x10,              /* BIOS teletype */
        0xeb,0xf7,              /* JMP LODSB */
        0xfa,0xf4,0xeb,0xfd    /* CLI, HLT, JMP HLT */
    };
    for(unsigned i=0;i<sizeof x86;i++)b[i]=x86[i];
    b[0x1a]=13;b[0x1b]=10;
    unsigned p=0x1c;
    for(unsigned j=0;j<12u;j++)b[p++]=(uint8_t)(label&&label[j]?label[j]:' ');
    const char *message=" is not a system disk\r\n";
    for(unsigned i=0;message[i];i++)b[p++]=(uint8_t)message[i];
    b[p]=0;
    b[510u]=0x55u;b[511u]=0xaau;
}
uint32_t port_init_index(const struct port_init_index_layout *o,
                         uint8_t b[512],port_init_write_block writer,void *ctx){
    if(!o || !b || !writer || !o->cluster || !o->max_files ||
       !o->boot_blocks || o->boot_blocks!=o->cluster ||
       !o->index_areas || !o->index_area_count ||
       !o->index_blocks || !o->mfd_blocks || !o->bitmap_blocks ||
       o->index_blocks<(o->max_files+4095u)/4096u+9u ||
       o->boot_lbn!=0u || o->max_files<9u ||
       o->bad_block_count>32u || (o->bad_block_count&&!o->bad_blocks))
        return PORT_INIT_MFD_BAD_ARGUMENT;
    const uint32_t ibm=(o->max_files+4095u)/4096u;
    if(o->index_area_count<2u ||
       o->index_areas[0].start_lbn!=o->boot_lbn ||
       o->index_areas[o->index_area_count-1u].start_lbn!=o->index_lbn)
        return PORT_INIT_MFD_BAD_ARGUMENT;
    make_non_system_bootblock(b,o->label);
    uint32_t rc=save(writer,ctx,o->boot_lbn,b);
    if(!(rc&1u))return rc;
    fill(b,512,0);
    set32(b,1u);set32(b+4u,o->real_home_lbn);
    set32(b+8u,o->alternate_index_lbn);
    set16(b+12u,513u);set16(b+14u,(uint16_t)o->cluster);
    set16(b+16u,2u);
    set16(b+18u,(uint16_t)(o->real_home_lbn-o->home2_lbn+
                             o->cluster*2u+1u));
    set16(b+20u,(uint16_t)(o->cluster*3u+1u));
    set16(b+22u,(uint16_t)(o->cluster*4u+1u));
    set32(b+24u,o->index_lbn);
    set32(b+28u,o->max_files);
    set16(b+32u,(uint16_t)ibm);set16(b+34u,9u);
    set32(b+44u,o->owner_uic);
    set16(b+52u,(uint16_t)o->protection);
    if(o->read_check)b[42u]|=1u;
    if(o->write_check)b[42u]|=2u;
    set16(b+54u,(uint16_t)o->file_protection);
    set16(b+56u,(uint16_t)o->record_protection);
    set32(b+60u,(uint32_t)o->creation_time);
    set32(b+64u,(uint32_t)(o->creation_time>>32u));
    b[68u]=o->window;b[69u]=o->lru_limit;
    set16(b+70u,(uint16_t)o->extension);
    set32(b+456u,o->serial);
    write_text(b+460u,12u,0);
    write_text(b+472u,12u,o->label);
    write_text(b+484u,12u,o->owner_name);
    write_text(b+496u,12u,"DECFILE11B  ");
    for(uint32_t i=1u;i<o->boot_blocks;i++){
        rc=home_block(b,writer,ctx);if(!(rc&1u))return rc;
    }
    set32(b,o->home1_lbn);
    for(uint32_t i=0;i<o->cluster;i++){
        rc=home_block(b,writer,ctx);if(!(rc&1u))return rc;
    }
    set32(b,o->home2_lbn);
    for(uint32_t i=0;i<o->cluster;i++){
        rc=home_block(b,writer,ctx);if(!(rc&1u))return rc;
    }
    fill(b,512,0);
    set32(b,0x1ffu);
    rc=save(writer,ctx,o->index_lbn,b);if(!(rc&1u))return rc;
    set32(b,0u);
    for(uint32_t j=1;j<ibm;j++){
        rc=save(writer,ctx,o->index_lbn+j,b);if(!(rc&1u))return rc;
    }
    /* INITIAL_HEADER from ININDX.B32, including all original FH2/FI2
     * fields, dates, protection and MFD backlink. */
    fill(b,512,0);
    b[0u]=38u;b[1u]=65u;b[2u]=255u;b[3u]=255u;
    set16(b+6u,513u);
    b[20u]=1u;set16(b+22u,512u);set16(b+36u,512u);
    set32(b+28u,0x00010000u);
    set16(b+34u,512u);
    set16(b+56u,(uint16_t)o->record_protection);
    set32(b+60u,o->owner_uic);
    set16(b+64u,(uint16_t)o->file_protection);
    set16(b+66u,4u);set16(b+68u,4u);
    set16(b+96u,1u);
    set32(b+98u,(uint32_t)o->creation_time);
    set32(b+102u,(uint32_t)(o->creation_time>>32u));
    set32(b+106u,(uint32_t)o->creation_time);
    set32(b+110u,(uint32_t)(o->creation_time>>32u));
    const uint32_t first_header=o->index_lbn+ibm;
    set_file(b,5u,"CORIMG.SYS;1");
    rc=write_header(writer,ctx,b,first_header+4u);
    if(!(rc&1u))return rc;
    set_file(b,7u,"CONTIN.SYS;1");
    rc=write_header(writer,ctx,b,first_header+6u);
    if(!(rc&1u))return rc;
    set_file(b,6u,"VOLSET.SYS;1");set16(b+22u,64u);set16(b+36u,64u);
    rc=write_header(writer,ctx,b,first_header+5u);
    if(!(rc&1u))return rc;
    set_file(b,8u,"BACKUP.SYS;1");
    rc=write_header(writer,ctx,b,first_header+7u);
    if(!(rc&1u))return rc;
    set_file(b,9u,"BADLOG.SYS;1");set16(b+22u,16u);set16(b+36u,16u);
    rc=write_header(writer,ctx,b,first_header+8u);
    if(!(rc&1u))return rc;
    set_file(b,1u,"INDEXF.SYS;1");set16(b+22u,512u);set16(b+36u,512u);
    set32(b+24u,(o->cluster*4u+o->index_blocks)<<16u);
    set32(b+28u,(o->cluster*4u+ibm+6u)<<16u);
    uint32_t start=o->index_areas[0].start_lbn;
    uint32_t count=o->index_areas[0].block_count;
    for(size_t i=1u;i<o->index_area_count;i++){
        const struct port_init_allocation *next=&o->index_areas[i];
        if(start+count==next->start_lbn){
            count+=next->block_count;
        }else{
            rc=make_pointer(b,count,start);
            if(!(rc&1u))return rc;
            start=next->start_lbn;count=next->block_count;
        }
    }
    rc=make_pointer(b,count,start);
    if(!(rc&1u))return rc;
    rc=write_header(writer,ctx,b,first_header);
    if(!(rc&1u))return rc;
    rc=save(writer,ctx,o->alternate_index_lbn,b);
    if(!(rc&1u))return rc;
    /* BADBLK.SYS, BITMAP.SYS and 000000.DIR: each clears the map. */
    fill(b+130u,380u,0u);b[58u]=0u;
    set_file(b,3u,"BADBLK.SYS;1");
    uint32_t bad_count=0u;
    for(size_t i=0;i<o->bad_block_count;i++)bad_count+=o->bad_blocks[i].block_count;
    set32(b+24u,bad_count<<16u);
    set32(b+28u,(bad_count+1u)<<16u);
    for(size_t i=0;i<o->bad_block_count;i++){
        rc=make_pointer(b,o->bad_blocks[i].block_count,
                        o->bad_blocks[i].start_lbn);
        if(!(rc&1u))return rc;
    }
    rc=write_header(writer,ctx,b,first_header+2u);
    if(!(rc&1u))return rc;
    fill(b+130u,380u,0u);b[58u]=0u;
    set_file(b,2u,"BITMAP.SYS;1");
    set32(b+52u,get32(b+52u)|(1u<<7u));
    set32(b+24u,o->bitmap_blocks<<16u);
    set32(b+28u,((o->volume_blocks/o->cluster+4095u)/4096u+2u)<<16u);
    rc=make_pointer(b,o->bitmap_blocks,o->bitmap_lbn);
    if(!(rc&1u))return rc;
    rc=write_header(writer,ctx,b,first_header+1u);
    if(!(rc&1u))return rc;
    fill(b+130u,380u,0u);b[58u]=0u;
    set_file(b,4u,"000000.DIR;1");
    set32(b+52u,get32(b+52u)|(1u<<13u));
    set16(b+64u,(uint16_t)(get16(b+64u)&~0x4444u));
    set32(b+24u,o->mfd_blocks<<16u);
    set32(b+28u,2u<<16u);
    b[20u]=2u;b[21u]=8u;
    rc=make_pointer(b,o->mfd_blocks,o->mfd_lbn);
    if(!(rc&1u))return rc;
    return write_header(writer,ctx,b,first_header+3u);
}
