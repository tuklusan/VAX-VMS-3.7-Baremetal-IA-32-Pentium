#include "vms_f11_acp87.h"
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static void wr16(uint8_t *p,unsigned x){p[0]=(uint8_t)x;p[1]=(uint8_t)(x>>8);}
static uint16_t rd16(const uint8_t *p){return (uint16_t)(p[0]|(uint16_t)p[1]<<8);}
struct fake {
    uint8_t header[512];
    uint8_t extension_header[512];
    unsigned allocation,deallocation,turns,maps,reads,invalidated,switched,posts,dismounts,mounts;
    unsigned fail_read_number;
    uint8_t window_write_access;
    uint32_t vbn,lbn;
    uint32_t extension_vbn,extension_lbn;
    FILE *disk;
};
static void *allocate(void *p,uint32_t bytes,uint32_t kind){
    struct fake *m=p;
    assert(kind==PORT_F11_87_FCBTYPE||kind==PORT_F11_87_WCBTYPE);
    m->allocation++;
    return calloc(1,bytes);
}
static uint32_t deallocate(void *p,void *memory,uint32_t bytes,uint32_t kind){
    struct fake *m=p;(void)bytes;
    assert((kind==PORT_F11_87_FCBTYPE||kind==PORT_F11_87_WCBTYPE)&&memory);m->deallocation++;
    free(memory);return 1u;
}
static uint32_t mount_kernel(void *p,uint32_t *bits,uint32_t mask){
    struct fake *m=p;m->mounts++;*bits|=mask;return 1u;
}
static uint32_t turn(void *p,struct port_f11_87_window *w,const uint8_t h[512],
                     uint32_t first,uint32_t number,struct port_f11_87_window **out){
    struct fake *m=p;assert(first==1&&number==1&&h);m->turns++;
    if(!w){w=calloc(1,sizeof(*w));assert(w);}
    w->write_access=m->window_write_access;*out=w;
    return 1;
}
static uint32_t map(void *p,uint32_t vbn,uint32_t *lbn){
    struct fake *m=p;m->maps++;m->vbn=vbn;
    *lbn=m->extension_vbn==vbn?m->extension_lbn:m->lbn;return 1;
}
static uint32_t read_lbn(void *p,uint32_t lbn,uint8_t h[512]){
    struct fake *m=p;m->reads++;
    if(m->fail_read_number==m->reads)return PORT_F11_87_IOERROR;
    if(lbn==m->lbn)memcpy(h,m->header,512);
    else if(m->extension_lbn&&lbn==m->extension_lbn)
        memcpy(h,m->extension_header,512);
    else {
        assert(m->disk&&fseek(m->disk,(long)((uint64_t)lbn*512u),SEEK_SET)==0);
        assert(fread(h,1u,512u,m->disk)==512u);
    }
    return 1;
}
static uint32_t switch_volume(void *p,uint16_t rvn){
    struct fake *m=p; m->switched++;assert(rvn==0||rvn==1);
    return 1;
}
static void invalidate(void *p,uint32_t lbn){struct fake *m=p;assert(lbn==m->lbn);m->invalidated++;}
static uint32_t post(void *p,struct port_f11_87_packet *pkt){struct fake *m=p;assert(pkt);m->posts++;return 1;}
static uint32_t dismount(void *p){struct fake *m=p;m->dismounts++;return 1;}

int main(int argc,char **argv){
    assert(argc==2);
    struct fake m={0};m.lbn=22;m.window_write_access=1u;
    struct port_f11_87_volume vol={0};vol.index_bitmap_blocks=1;vol.cluster=1;
    vol.reserved_files=16;vol.default_window=4;
    struct port_f11_87_context c={0};c.volume=&vol;c.ops=(struct port_f11_87_ops){
        .context=&m,.alloc=allocate,.dealloc=deallocate,.kernel_mount=mount_kernel,
        .turn_window=turn,.map_vbn=map,
        .read_lbn=read_lbn,.switch_volume=switch_volume,.invalidate=invalidate,
        .post_io=post,.check_dismount=dismount
    };c.ucb=(void *)(uintptr_t)0x1234;
    uint32_t d=0;
    assert(port_f11a_mount(&c,2,&d)==PORT_F11_87_WRONGACP&&d==0);
    assert(port_f11a_mount(&c,1,&d)==1&&d==(1u<<19|1u<<3));
    assert(port_f11b_mount(&c,2,&d)==1&&m.mounts==2u);
    c.ops.kernel_mount=NULL;
    assert(port_f11b_mount(&c,2,&d)==PORT_F11_87_BADPARAM);
    c.ops.kernel_mount=mount_kernel;
    uint8_t name[10]={0};
    wr16(name,8u*1600u+5u*40u+12u);
    wr16(name+2,12u*1600u+15u*40u);
    wr16(name+6,20u*1600u+24u*40u+20u);
    wr16(name+8,12u);
    uint8_t ascii[24]={0};
    assert(port_f11a_make_string(name,ascii)==12u&&!memcmp(ascii,"HELLO.TXT;12",12));
    assert(port_f11b_make_string(name,ascii)==12u);
    uint8_t o1[512]={0};o1[1]=30;wr16(o1+2,5);wr16(o1+4,3);wr16(o1+6,257);
    unsigned off=60;o1[off+6]=1;o1[off+7]=3;o1[off+8]=2;o1[off+9]=4;
    o1[off+10]=0;o1[off+11]=2;wr16(o1+off+12,345u);
    assert(port_f11a_checksum(o1)==0u);
    struct port_f11_86_fid fid1={5,3,0,0};uint32_t st=0;
    assert(port_f11a_check_header(o1,fid1,&st)==1u&&st==1);
    assert(port_f11a_check_header(o1,(struct port_f11_86_fid){6,3,0,0},&st)==2u&&st==PORT_F11_87_FILENUMCHK);
    wr16(o1+2,0);
    assert(port_f11a_check_header(o1,fid1,&st)==2u&&st==PORT_F11_87_NOSUCHFILE);
    wr16(o1+2,5);port_f11a_checksum(o1);
    struct port_f11_87_fcb f1={0};
    assert(port_f11a_init_fcb(&f1,o1,100)==1&&f1.file_blocks==3&&f1.fid.number==5);
    assert(port_f11a_update_fcb(&c,o1,100)==1u);
    c.primary=&f1;f1.file_blocks=31;
    assert(port_f11a_update_fcb(&c,o1,101)==1u&&f1.file_blocks==31&&f1.header_lbn==101);
    c.primary=0;
    struct port_f11_87_fcb *created=port_f11a_create_fcb(&c,o1,66);
    assert(created&&created->start_vbn==1u&&created->header_lbn==66&&m.allocation==1u);
    created->type=PORT_F11_87_FCBTYPE;
    vol.head=created;vol.tail=created;
    assert(port_f11a_search_fcb(&vol,fid1)==created);
    assert(port_f11a_search_fcb(&vol,(struct port_f11_86_fid){7,1,0,0})==0);
    vol.head=0;vol.tail=0;created->next=0;created->prev=0;
    struct port_f11_87_window *window=port_f11a_create_window(&c,15,4,o1,1234,created);
    assert(window&&window->read&&window->write&&window->pointer_count==4&&
           window->fcb==created&&window->pid==1234&&window->ucb==c.ucb);
    window->no_write=1;window->no_truncate=1;
    struct port_f11_87_window *writeback=0;
    assert(port_f11a_make_access(&c,created,window,&writeback)==1u);
    assert(writeback==window&&created->accesses==1u&&created->writers==1u&&
           created->no_writers==1u&&created->no_truncate==1u&&vol.transactions==1);
    uint8_t fib[64]={0};
    wr16(fib+10,0xffffu);wr16(fib+12,0xffffu);
    struct port_f11_87_fib_result fibout;
    c.window=window;
    assert(port_f11a_get_fib(&c,fib,32,0,&fibout)==1u);
    assert(rd16(fibout.bytes+10)==4u&&rd16(fibout.bytes+12)==4u);
    assert(rd16(fibout.bytes+4)==5u);
    struct port_f11_87_packet p={0};p.io_func=2;p.byte_count=100;p.flags=PORT_F11_87_IRP_COMPLX;
    uint8_t fibwrite[16]={0};
    struct port_f11_86_abd abd={0,2,fibwrite,16,0};
    assert(port_f11a_io_done(&c,&p,1,512,&abd,fib,16)==1u);
    assert(p.status==1u&&p.value==512u&&m.posts==1u&&m.dismounts==1u);
    assert(vol.transactions==0&&abd.name_count==0);
    assert((p.flags & PORT_F11_87_IRP_FUNC)!=0&&p.byte_count==4);
    p.io_func=PORT_F11_87_IO_READPBLK;p.flags=0x10u;p.saved_page=77;p.user_page=88;
    p.byte_count=200;p.original_bytes=14;
    assert(port_f11b_io_done(&c,&p,12,0,0,0,0)==1u);
    assert(p.user_page==77u&&p.byte_count==14u&&!(p.flags&0x10u));

    /* Actual INDEXF.SYS header in the generated Files-11 disk. */
    FILE *fp=fopen(argv[1],"rb");assert(fp);m.disk=fp;
    assert(fseek(fp,22L*512L,SEEK_SET)==0);
    assert(fread(m.header,1,512,fp)==512u);
    assert(port_f11b_checksum(m.header)==1u);
    struct port_f11_86_fid fid2={rd16(m.header+8),rd16(m.header+10),
                                  rd16(m.header+12),m.header[13]};
    assert(port_f11b_check_header2(m.header,fid2,0,&st)==1u&&st==1u);
    uint8_t wrong_checksum[512];memcpy(wrong_checksum,m.header,512);
    wrong_checksum[30]^=0x10u;
    assert(port_f11b_check_header2(wrong_checksum,fid2,0,&st)==2u&&st==PORT_F11_87_BADCHKSUM);
    memcpy(wrong_checksum,m.header,512);wrong_checksum[3]=0;
    assert(port_f11b_check_header2(wrong_checksum,fid2,0,&st)==0u&&st==PORT_F11_87_BADFILEHDR);
    assert(port_f11b_check_header2(m.header,
        (struct port_f11_86_fid){fid2.number,(uint16_t)(fid2.sequence+1u),fid2.volume,0},
        0,&st)==4u&&st==PORT_F11_87_NOSUCHFILE);
    struct port_f11_87_fcb f2={0};
    assert(port_f11b_init_fcb2(&f2,m.header,22,&vol)==1u&&f2.file_blocks==3u);
    assert(f2.fid.number==fid2.number&&f2.header_lbn==22);
    uint8_t invalid_fcb_header[512];memcpy(invalid_fcb_header,m.header,512);
    invalid_fcb_header[1]=0u;
    unsigned fcb_alloc_before=m.allocation,fcb_free_before=m.deallocation;
    assert(port_f11b_create_fcb(&c,invalid_fcb_header,22)==0);
    assert(c.status==PORT_F11_87_BADFILEHDR&&
           m.allocation==fcb_alloc_before+1u&&
           m.deallocation==fcb_free_before+1u);
    struct port_f11_87_fcb *new_fcb=port_f11b_create_fcb(&c,m.header,22);
    assert(new_fcb&&new_fcb->file_blocks==3u);
    assert(port_f11b_search_fcb(&vol,fid2)==0);
    assert(port_f11b_make_pointer(m.header,(uint16_t[]){80u},1u,22u,0u)==0u);
    uint8_t ptrhead[512]={0};ptrhead[0]=40;ptrhead[1]=40;ptrhead[2]=70;ptrhead[3]=70;ptrhead[7]=2;
    uint16_t position=80u;
    assert(port_f11b_make_pointer(ptrhead,&position,5,20000,0u)==1u&&position==84u);
    assert(port_f11b_make_pointer(ptrhead,&position,164,100000,0x0001u)==1u&&position==90u);
    assert(port_f11b_make_pointer(ptrhead,&position,20000,7,0u)==1u&&position==98u);
    uint16_t failed_position=position;
    uint8_t prior_used=ptrhead[58];
    ptrhead[2]=ptrhead[1]+(uint8_t)(prior_used);
    assert(!port_f11b_make_pointer(ptrhead,&position,100,30,0u));
    assert(position==failed_position&&ptrhead[58]==prior_used);
    ptrhead[2]=70u;
    uint32_t total=0;
    assert(port_f11b_file_size(ptrhead,&total)==1u&&total==20169u);
    uint32_t actual=0;
    assert(port_f11b_read_header(&c,&fid2,0,(uint8_t[512]){0},&actual)==1u);
    assert(actual==22u&&m.maps==1u&&m.reads==1u&&m.switched==1u);
    assert(m.vbn==fid2.number+vol.index_bitmap_blocks+4u*vol.cluster);
    assert(port_f11a_read_header(&c,&fid1,0,(uint8_t[512]){0},&actual)==PORT_F11_87_FILESTRUCT);
    assert(m.invalidated==1u);
    struct port_f11_87_window *win2=port_f11b_create_window(&c,0,0,m.header,99,new_fcb);
    assert(win2&&win2->pointer_count==4&&win2->read&&win2->write);
    c.primary=new_fcb;
    assert(port_f11b_update_fcb(&c,m.header,23)==1u&&new_fcb->header_lbn==23u);
    struct port_f11_87_window *wb2=0;
    assert(port_f11b_make_access(&c,new_fcb,win2,&wb2)==1u);
    assert(wb2==win2&&new_fcb->writers==1u);
    c.window=win2;
    memset(fib,0,sizeof fib);
    fib[23]=0x80u;
    assert(port_f11b_get_fib(&c,fib,20,2,&fibout)==1u);
    assert((fibout.bytes[23]&0x80u)==0u);
    assert(rd16(fibout.bytes+4)==fid2.number);
    assert(port_f11b_io_done(&c,&p,1,0,0,0,0)==1u);
    const struct port_f11_87_deaccess_request no_attributes={0};
    assert(port_f11b_deaccess(&c,&no_attributes)==PORT_F11_87_INCOMPLETE);
    m.window_write_access=0u;
    struct port_f11_87_fcb *read_fcb=port_f11b_create_fcb(&c,m.header,22);
    assert(read_fcb);
    struct port_f11_87_window *read_window=
        port_f11b_create_window(&c,0,0,m.header,100,read_fcb);
    assert(read_window&&read_window->read&&!read_window->write);
    struct port_f11_87_window *readback=0;
    struct port_f11_87_context close_context={
        .volume=&vol,.ops=c.ops,.ucb=c.ucb,.primary=read_fcb};
    assert(port_f11b_make_access(&close_context,read_fcb,read_window,
                                 &readback)==1u);
    close_context.window=read_window;
    uint8_t acp_bytes[700],expected[700];
    uint32_t acp_completed=0u;
    unsigned reads_before_acp=m.reads;
    assert(port_f11b_read_vbn(&close_context,read_fcb,300u,
        sizeof acp_bytes,acp_bytes,sizeof acp_bytes,&acp_completed)==
        PORT_F11_87_NORMAL&&acp_completed==sizeof acp_bytes);
    assert(m.reads==reads_before_acp+3u);
    unsigned reads_after_acp=m.reads;
    assert(port_f11b_read_vbn(&close_context,read_fcb,
        read_fcb->file_blocks*512u,1u,acp_bytes,sizeof acp_bytes,
        &acp_completed)==PORT_F11_87_INCOMPLETE&&acp_completed==0u&&
        m.reads==reads_after_acp+1u);
    reads_after_acp=m.reads;
    assert(port_f11b_read_vbn(&close_context,read_fcb,0u,0u,0,0u,
        &acp_completed)==PORT_F11_87_NORMAL&&acp_completed==0u&&
        m.reads==reads_after_acp);
    m.fail_read_number=m.reads+1u;
    assert(port_f11b_read_vbn(&close_context,read_fcb,300u,700u,
        acp_bytes,sizeof acp_bytes,&acp_completed)==PORT_F11_87_IOERROR&&
        acp_completed==0u);
    m.fail_read_number=m.reads+3u;
    assert(port_f11b_read_vbn(&close_context,read_fcb,300u,700u,
        acp_bytes,sizeof acp_bytes,&acp_completed)==PORT_F11_87_IOERROR&&
        acp_completed==212u);
    m.fail_read_number=0u;
    uint32_t first_lbn=0u,second_lbn=0u,third_lbn=0u;
    struct port_f11_86_map actual_map={m.header+2u*m.header[1],
        2u*m.header[58],0u,0u,0u};
    uint32_t next_vbn=1u;
    while(actual_map.offset<actual_map.length){
        assert(port_f11b_get_map_pointer(&actual_map));
        if(!actual_map.blocks)continue;
        if(1u>=next_vbn&&1u-next_vbn<actual_map.blocks)
            first_lbn=actual_map.lbn+(1u-next_vbn);
        if(2u>=next_vbn&&2u-next_vbn<actual_map.blocks)
            second_lbn=actual_map.lbn+(2u-next_vbn);
        if(3u>=next_vbn&&3u-next_vbn<actual_map.blocks)
            third_lbn=actual_map.lbn+(3u-next_vbn);
        next_vbn+=actual_map.blocks;
    }
    assert(first_lbn&&second_lbn&&third_lbn);
    assert(fseek(fp,(long)((uint64_t)first_lbn*512u+300u),SEEK_SET)==0&&
           fread(expected,1u,212u,fp)==212u);
    assert(fseek(fp,(long)((uint64_t)second_lbn*512u),SEEK_SET)==0&&
           fread(expected+212u,1u,sizeof expected-212u,fp)==sizeof expected-212u);
    assert(!memcmp(acp_bytes,expected,sizeof expected));
    /* FIL$RDCHKFILHDR follows FH2$W_EXT_FID and advances the segment. */
    m.extension_lbn=7000u;
    m.extension_vbn=6u+vol.index_bitmap_blocks+4u*vol.cluster;
    memcpy(m.extension_header,m.header,512);
    wr16(m.extension_header+4u,1u);
    wr16(m.extension_header+8u,6u);
    wr16(m.extension_header+10u,1u);
    wr16(m.extension_header+12u,0u);
    memset(m.extension_header+14u,0,6u);
    port_f11b_checksum(m.extension_header);
    wr16(m.header+14u,6u);
    wr16(m.header+16u,1u);
    wr16(m.header+18u,0u);
    m.header[19u]=0u;
    port_f11b_checksum(m.header);
    uint8_t extension_bytes[64],extension_expected[64];
    unsigned reads_before_extension=m.reads;
    assert(port_f11b_read_vbn(&close_context,read_fcb,
        read_fcb->file_blocks*512u,sizeof extension_bytes,extension_bytes,
        sizeof extension_bytes,&acp_completed)==PORT_F11_87_NORMAL&&
        acp_completed==sizeof extension_bytes&&m.reads==reads_before_extension+4u);
    assert(fseek(fp,(long)((uint64_t)first_lbn*512u),SEEK_SET)==0&&
        fread(extension_expected,1u,sizeof extension_expected,fp)==
             sizeof extension_expected);
    assert(!memcmp(extension_bytes,extension_expected,sizeof extension_bytes));
    uint8_t boundary_bytes[64],boundary_expected[64];
    unsigned reads_before_boundary=m.reads;
    assert(port_f11b_read_vbn(&close_context,read_fcb,
        2u*512u+480u,sizeof boundary_bytes,boundary_bytes,
        sizeof boundary_bytes,&acp_completed)==PORT_F11_87_NORMAL&&
        acp_completed==sizeof boundary_bytes&&
        m.reads==reads_before_boundary+5u);
    assert(fseek(fp,(long)((uint64_t)third_lbn*512u+480u),SEEK_SET)==0&&
        fread(boundary_expected,1u,32u,fp)==32u);
    assert(fseek(fp,(long)((uint64_t)first_lbn*512u),SEEK_SET)==0&&
        fread(boundary_expected+32u,1u,32u,fp)==32u);
    assert(!memcmp(boundary_bytes,boundary_expected,sizeof boundary_bytes));
    assert(port_f11b_read_vbn(&close_context,read_fcb,
        read_fcb->file_blocks*2u*512u,1u,acp_bytes,sizeof acp_bytes,
        &acp_completed)==PORT_F11_87_INCOMPLETE&&acp_completed==0u);
    /* A self-referential extension fails the source segment check. */
    memcpy(m.extension_header+14u,m.extension_header+8u,6u);
    port_f11b_checksum(m.extension_header);
    assert(port_f11b_read_vbn(&close_context,read_fcb,
        read_fcb->file_blocks*2u*512u,1u,acp_bytes,
        sizeof acp_bytes,&acp_completed)==PORT_F11_87_BADFILEHDR);
    unsigned free_before_close=m.deallocation;
    unsigned transactions_before_close=vol.transactions;
    assert(port_f11b_deaccess(&close_context,&no_attributes)==PORT_F11_87_NORMAL);
    assert(!close_context.primary&&!close_context.window&&
           !close_context.cleanup_deaccess&&close_context.open_count==0u&&
           vol.transactions+1u==transactions_before_close&&
           read_fcb->accesses==0u&&
           !read_fcb->windows&&!read_fcb->window_tail&&
           m.deallocation==free_before_close+1u);
    /* An extended ODS-2 FID keeps the file-number extension and RVN apart. */
    uint8_t extended_header[512];memcpy(extended_header,m.header,512);
    extended_header[13]=2u;
    port_f11b_checksum(extended_header);
    vol.extended_fid=1u;
    struct port_f11_87_fcb efcb={0};
    assert(port_f11b_init_fcb2(&efcb,extended_header,22,&vol)==1u);
    assert(efcb.fid.number_extension==2u&&efcb.fid.volume==0x0200u);
    vol.extended_fid=0u;
    /* An invalid second-stage header retains no successful load address. */
    uint8_t invalid_header[512];memcpy(invalid_header,m.header,512);
    invalid_header[30]^=0x11u;
    memcpy(m.header,invalid_header,512);
    uint32_t before=m.invalidated;
    assert(port_f11b_read_header(&c,&fid2,0,(uint8_t[512]){0},&actual)==PORT_F11_87_BADCHKSUM);
    assert(m.invalidated==before+1u);
    memcpy(m.header,extended_header,512);
    /* A valid ODS-1 header is read from the same active disk callback. */
    memcpy(m.header,o1,512);
    assert(port_f11a_read_header(&c,&fid1,0,(uint8_t[512]){0},&actual)==1u);
    assert(m.vbn==fid1.number+vol.index_bitmap_blocks+2u);
    free(win2);
    free(new_fcb);
    free(window);
    free(created);
    fclose(fp);
    puts("F11A/F11B M87: mount, header, FCB, window, FIB, map, I/O completion PASS");
    return 0;
}
