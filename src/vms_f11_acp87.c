#include "vms_f11_acp87.h"

static uint16_t le16(const uint8_t *p){return (uint16_t)((uint16_t)p[0]|((uint16_t)p[1]<<8));}
static uint32_t le32(const uint8_t *p){return (uint32_t)le16(p)|((uint32_t)le16(p+2)<<16);}
static void wr16(uint8_t *p,uint16_t v){p[0]=(uint8_t)v;p[1]=(uint8_t)(v>>8);}
static void wr32(uint8_t *p,uint32_t v){wr16(p,(uint16_t)v);wr16(p+2,(uint16_t)(v>>16));}
static void copy(uint8_t *dst,const uint8_t *src,size_t n){for(size_t i=0;i<n;i++)dst[i]=src[i];}
static void zero(uint8_t *p,size_t n){for(size_t i=0;i<n;i++)p[i]=0;}
static uint32_t swapped(uint32_t v){return (v<<16)|(v>>16);}
static uint32_t status_fail(uint32_t *s,uint32_t code){if(s)*s=code;return 0;}
static void add_list(struct port_f11_87_volume *v,struct port_f11_87_fcb *p){
    if(!v||!p)return;
    p->prev=v->tail;p->next=0;
    if(v->tail)v->tail->next=p;else v->head=p;
    v->tail=p;
}

static uint32_t do_mount(struct port_f11_87_context *c,uint8_t required,
                         uint8_t supplied,uint32_t *device_bits){
    if(!c||!device_bits)return PORT_F11_87_BADPARAM;
    if(supplied!=required)return PORT_F11_87_WRONGACP;
    if(!c->ops.kernel_mount)return PORT_F11_87_BADPARAM;
    /* KERNEL_CALL (SET_MOUNTED) changes UCB characteristics. */
    return c->ops.kernel_mount(c->ops.context,device_bits,(1u<<19)|(1u<<3));
}
uint32_t port_f11a_mount(struct port_f11_87_context *c,uint8_t actual,
                         uint32_t *bits){return do_mount(c,1u,actual,bits);}
uint32_t port_f11b_mount(struct port_f11_87_context *c,uint8_t actual,
                         uint32_t *bits){return do_mount(c,2u,actual,bits);}

static struct port_f11_87_fcb *search_fcb(struct port_f11_87_volume *v,
                                         struct port_f11_86_fid id,int extended){
    if(!v)return 0;
    struct port_f11_87_fcb *prev=0,*p=v->head;
    unsigned scanned=0;
    while(p){
        if(p->prev!=prev||p->type!=PORT_F11_87_FCBTYPE||scanned++>=65536u){
            /* A broken VCB chain is a fatal FCP consistency error. */
            __builtin_trap();
        }
        if(p->fid.number==id.number && (!extended||p->fid.number_extension==id.number_extension))return p;
        prev=p;p=p->next;
    }
    return 0;
}
struct port_f11_87_fcb *port_f11a_search_fcb(struct port_f11_87_volume *v,
                                             struct port_f11_86_fid id){return search_fcb(v,id,0);}
struct port_f11_87_fcb *port_f11b_search_fcb(struct port_f11_87_volume *v,
                                             struct port_f11_86_fid id){return search_fcb(v,id,1);}

static size_t make_string(const uint8_t nb[10],uint8_t out[24]){
    if(!nb||!out)return 0;
    size_t n=0;
    for(unsigned field=0;field<2;field++){
        unsigned words=field?1u:3u;
        for(unsigned j=0;j<words;j++){
            uint16_t rad=le16(nb+2u*(field?3u:j));
            uint8_t v[3]={(uint8_t)(rad/1600u),(uint8_t)((rad/40u)%40u),(uint8_t)(rad%40u)};
            for(unsigned k=0;k<3;k++){
                if(!v[k])continue;
                out[n++]=v[k]<30u?(uint8_t)('A'+v[k]-1u):(uint8_t)('0'+v[k]-30u);
            }
        }
        out[n++]=field?';':'.';
    }
    uint16_t ver=le16(nb+8);
    /* $FAO !SW produces the signed decimal version number. */
    int32_t value=(int16_t)ver;
    if(value<0){out[n++]='-';value=-value;}
    uint32_t d=(uint32_t)value;
    uint8_t temp[5];unsigned k=0;
    do{temp[k++]=(uint8_t)('0'+d%10u);d/=10u;}while(d);
    while(k)out[n++]=temp[--k];
    return n;
}
size_t port_f11a_make_string(const uint8_t x[10],uint8_t y[24]){return make_string(x,y);}
size_t port_f11b_make_string(const uint8_t x[10],uint8_t y[24]){return make_string(x,y);}

uint32_t port_f11a_check_header(uint8_t h[512],struct port_f11_86_fid fid,uint32_t *st){
    if(!h)return status_fail(st,PORT_F11_87_BADPARAM);
    if(le16(h+6)!=257u)return status_fail(st,PORT_F11_87_FILESTRUCT);
    unsigned map=2u*h[1];
    if(map+10u>510u)return status_fail(st,PORT_F11_87_BADFILEHDR);
    if(h[map+1]!=0||h[map+6]!=1||h[map+7]!=3)
        return status_fail(st,PORT_F11_87_FILESTRUCT);
    unsigned capacity=(510u-map-10u)/2u;
    if(h[map+8]>h[map+9]||h[map+9]>capacity)
        return status_fail(st,PORT_F11_87_BADFILEHDR);
    if(le16(h+2)==0){if(st)*st=PORT_F11_87_NOSUCHFILE;
        return 2;}
    if(!port_f11a_checksum(h)){if(st)*st=PORT_F11_87_BADCHKSUM;return 2;}
    if(le16(h+2)!=fid.number){if(st)*st=PORT_F11_87_FILENUMCHK;
        return 2;}
    if(le16(h+4)!=fid.sequence){if(st)*st=PORT_F11_87_NOSUCHFILE;
        return 2;}
    if(st)*st=PORT_F11_87_NORMAL;
    return 1;
}
uint32_t port_f11b_check_header2(uint8_t h[512],struct port_f11_86_fid fid,
                                 int extended,uint32_t *st){
    if(!h)return status_fail(st,PORT_F11_87_BADPARAM);
    if(h[7]!=2)return status_fail(st,PORT_F11_87_FILESTRUCT);
    if(h[0]<38||h[1]<h[0]||h[2]<h[1]||h[3]<h[2]||
       h[58]>(unsigned)(h[2]-h[1]))
        return status_fail(st,PORT_F11_87_BADFILEHDR);
    if(le16(h+8)==0 && (!extended || h[13]==0)){
        if(st)*st=PORT_F11_87_NOSUCHFILE;
        return 2;
    }
    if(!port_f11b_checksum(h)){if(st)*st=PORT_F11_87_BADCHKSUM;return 2;}
    if(le16(h+8)!=fid.number || (extended && h[13]!=fid.number_extension)){
        if(st)*st=PORT_F11_87_FILENUMCHK;
        return 2;
    }
    if(le16(h+10)!=fid.sequence){if(st)*st=PORT_F11_87_NOSUCHFILE;return 4;}
    if(st)*st=PORT_F11_87_NORMAL;
    return 1;
}

static uint32_t fill_fcb1(struct port_f11_87_fcb *f,const uint8_t h[512],uint32_t lbn,
                          int refresh){
    if(!f||!h)return PORT_F11_87_BADPARAM;
    unsigned map=2u*h[1];
    if(map+10u>510u)return PORT_F11_87_BADFILEHDR;
    uint32_t size=0;
    if(!refresh){
        if(h[map+8]>h[map+9]||(h[map+8]&1u)||map+10u+2u*h[map+8]>510u)
            return PORT_F11_87_BADFILEHDR;
        for(unsigned i=0;i<h[map+8];i+=2u)
            size+=(uint32_t)h[map+10u+2u*i+1u]+1u;
        f->file_blocks=size;
    }
    f->header_lbn=lbn;f->fid.number=le16(h+2);f->fid.sequence=le16(h+4);
    f->owner=le16(h+8);f->protection=le16(h+10);
    if(h[13]&16u)f->spool=1;
    f->segment=h[map];
    f->contiguous=(h[12]&0x80u)!=0;
    f->first_lbn=0;
    /* FM1 pointer begins with high LBN byte followed by count byte. */
    if(f->contiguous)f->first_lbn=((uint32_t)h[map+10]<<16)|le16(h+map+12);
    uint32_t ef=swapped(le32(h+14+8));
    if(ef&&le16(h+14+12)==0)ef--;
    f->end_block=ef>f->file_blocks?f->file_blocks:ef;
    return PORT_F11_87_NORMAL;
}
uint32_t port_f11a_init_fcb(struct port_f11_87_fcb *f,const uint8_t h[512],uint32_t lbn){
    return fill_fcb1(f,h,lbn,0);
}
static uint32_t fill_fcb2(struct port_f11_87_fcb *f,const uint8_t h[512],uint32_t lbn,
                          struct port_f11_87_volume *v){
    if(!f||!h||!v)return PORT_F11_87_BADPARAM;
    unsigned start=2u*h[1],end=start+2u*h[58];
    if(start<76u||end>2u*h[2]||end>510u)return PORT_F11_87_BADFILEHDR;
    f->header_lbn=lbn;f->fid.number=le16(h+8);f->fid.sequence=le16(h+10);
    f->fid.volume=le16(h+12);f->fid.number_extension=h[13];
    if(v->extended_fid){
        uint8_t rvn=(uint8_t)f->fid.volume;
        if(rvn==0)rvn=(uint8_t)v->current_rvn;
        if(rvn==1&&v->current_rvn==0)rvn=0;
        f->fid.volume=(uint16_t)((f->fid.volume&0xff00u)|rvn);
    }else{
        if(f->fid.volume==0)f->fid.volume=v->current_rvn;
        if(f->fid.volume==1&&v->current_rvn==0)f->fid.volume=0;
    }
    f->segment=le16(h+4);f->owner=le32(h+60);f->protection=le16(h+64);
    f->versions=le16(h+20+30);
    if(le32(h+52)&(1u<<12))f->spool=1;
    if(f->directory&&!(le32(h+52)&(1u<<13))){
        f->directory=0;v->directory_lru++;
        /* Removal of an idle FCB from the VCB list is done by the caller. */
        if(f->accesses==0 && (f->prev||f->next||v->head==f)){
            if(f->prev)f->prev->next=f->next;else v->head=f->next;
            if(f->next)f->next->prev=f->prev;else v->tail=f->prev;
            f->prev=f->next=0;
        }
    }
    uint32_t ef=swapped(le32(h+20+8));
    if(ef&&le16(h+20+12)==0)ef--;
    f->end_block=ef>f->file_blocks?f->file_blocks:ef;
    f->contiguous=(le32(h+52)&(1u<<7))!=0;
    f->first_lbn=0;
    if(f->contiguous&&start<end){
        struct port_f11_86_map m={h+start,(unsigned)(end-start),0,0,0};
        if(port_f11b_get_map_pointer(&m))f->first_lbn=m.lbn;
        else return PORT_F11_87_BADFILEHDR;
    }
    return PORT_F11_87_NORMAL;
}
uint32_t port_f11b_fill_fcb(struct port_f11_87_fcb *f,const uint8_t h[512],
                            uint32_t lbn,struct port_f11_87_volume *v){
    return fill_fcb2(f,h,lbn,v);
}
uint32_t port_f11b_init_fcb2(struct port_f11_87_fcb *f,const uint8_t h[512],
                             uint32_t lbn,struct port_f11_87_volume *v){
    if(!f||!h||!v)return PORT_F11_87_BADPARAM;
    uint32_t size;
    uint32_t st=port_f11b_file_size(h,&size);
    if(st!=PORT_F11_86_NORMAL)return PORT_F11_87_BADFILEHDR;
    f->file_blocks=size;
    return fill_fcb2(f,h,lbn,v);
}
static struct port_f11_87_fcb *create_fcb(struct port_f11_87_context *c,
                                          const uint8_t h[512],uint32_t lbn,int v2){
    if(!c||!c->volume||!c->ops.alloc||!h)return 0;
    struct port_f11_87_fcb *f=c->ops.alloc(c->ops.context,sizeof(*f),PORT_F11_87_FCBTYPE);
    if(!f)return 0;
    zero((uint8_t *)f,sizeof(*f));
    f->type=PORT_F11_87_FCBTYPE;f->start_vbn=1;
    uint32_t st=v2?port_f11b_init_fcb2(f,h,lbn,c->volume):port_f11a_init_fcb(f,h,lbn);
    if(st!=PORT_F11_87_NORMAL){
        c->status=(uint16_t)st;
        if(c->ops.dealloc&&
           !(c->ops.dealloc(c->ops.context,f,sizeof(*f),PORT_F11_87_FCBTYPE)&1u))
            c->status=PORT_F11_87_BADPARAM;
        return 0;
    }
    return f;
}
struct port_f11_87_fcb *port_f11a_create_fcb(struct port_f11_87_context *c,
                                               const uint8_t h[512],uint32_t lbn){
    return create_fcb(c,h,lbn,0);
}
struct port_f11_87_fcb *port_f11b_create_fcb(struct port_f11_87_context *c,
                                               const uint8_t h[512],uint32_t lbn){
    return create_fcb(c,h,lbn,1);
}
uint32_t port_f11a_update_fcb(struct port_f11_87_context *c,const uint8_t h[512],uint32_t lbn){
    if(!c)return PORT_F11_87_BADPARAM;
    return c->primary?fill_fcb1(c->primary,h,lbn,1):PORT_F11_87_NORMAL;
}
uint32_t port_f11b_update_fcb(struct port_f11_87_context *c,const uint8_t h[512],uint32_t lbn){
    if(!c)return PORT_F11_87_BADPARAM;
    return c->primary?fill_fcb2(c->primary,h,lbn,c->volume):PORT_F11_87_NORMAL;
}

uint32_t port_f11b_make_pointer(uint8_t h[512],uint16_t *position,
                                uint32_t count,uint32_t lbn,uint16_t placement){
    if(!h||!position)return 0;
    if(!count||count>(1u<<30))__builtin_trap();
    unsigned bytes=count<=256u&&lbn<(1u<<22)?4u:count<=16384u?6u:8u;
    unsigned start=2u*h[1],used=2u*h[58],capacity=2u*h[2];
    unsigned pos=*position;
    if(start<76u||capacity<start||capacity>510u||used>capacity-start||
       pos!=start+used||pos+bytes+(placement?2u:0u)>capacity||
       (unsigned)h[58]+(bytes+(placement?2u:0u))/2u>255u)
        return 0;
    /* CHECK_POINTER increments in-use only after space is available. */
    h[58]=(uint8_t)(h[58]+(bytes+(placement?2u:0u))/2u);
    if(placement){wr16(h+pos,placement);pos+=2u;}
    if(bytes==4u){wr16(h+pos,(uint16_t)(0x4000u|((lbn>>16&63u)<<8)|((count-1u)&255u)));wr16(h+pos+2,(uint16_t)lbn);}
    else if(bytes==6u){wr16(h+pos,(uint16_t)(0x8000u|((count-1u)&0x3fffu)));wr32(h+pos+2,lbn);}
    else {wr32(h+pos,((count-1u)<<16)|(0xc000u|((count-1u)>>16 & 0x3fffu)));wr32(h+pos+4,lbn);}
    *position=(uint16_t)(pos+bytes);
    return 1;
}

/* RDHEDR: INDEXF.SYS VBN mapping, header I/O, and FID checking. */
static uint32_t read_header(struct port_f11_87_context *c,
                            const struct port_f11_86_fid *id,
                            struct port_f11_87_fcb *f,uint8_t result[512],
                            uint32_t *lbn_out,int level2){
    if(!c||!c->volume||!result||!lbn_out||(!id&&!f)||!c->ops.read_lbn)
        return PORT_F11_87_BADPARAM;
    struct port_f11_86_fid fid=id?*id:f->fid;
    uint32_t status=PORT_F11_87_NORMAL,lbn=0;
    if(level2){
        if(!c->ops.switch_volume)return PORT_F11_87_BADPARAM;
        status=c->ops.switch_volume(c->ops.context,f?f->fid.volume:fid.volume);
        if(!(status&1u))return status;
    }
    if(f){lbn=f->header_lbn;}
    else {
        uint32_t vbn=fid.number;
        if(level2){
            if(c->volume->extended_fid)vbn|=(uint32_t)fid.number_extension<<16;
            if(!vbn)return PORT_F11_87_NOSUCHFILE;
            uint64_t value=(uint64_t)vbn+c->volume->index_bitmap_blocks+4u*c->volume->cluster;
            if(value>UINT32_MAX)return PORT_F11_87_BADFILEHDR;
            vbn=(uint32_t)value;
        }else{
            if(!vbn)return PORT_F11_87_NOSUCHFILE;
            uint64_t value=(uint64_t)vbn+c->volume->index_bitmap_blocks+2u;
            if(value>UINT32_MAX)return PORT_F11_87_BADFILEHDR;
            vbn=(uint32_t)value;
        }
        if(!c->ops.map_vbn)return PORT_F11_87_BADPARAM;
        status=c->ops.map_vbn(c->ops.context,vbn,&lbn);
        if(!(status&1u))return status;
    }
    if(lbn==UINT32_MAX)return PORT_F11_87_NOSUCHFILE;
    status=c->ops.read_lbn(c->ops.context,lbn,result);
    if(!(status&1u))return status;
    uint32_t header_status=PORT_F11_87_NORMAL;
    uint32_t valid=level2?
        port_f11b_check_header2(result,fid,c->volume->extended_fid,&header_status):
        port_f11a_check_header(result,fid,&header_status);
    if(valid!=1u){
        /* ODS-2 sequence mismatch retains a reusable cached header. */
        if(!level2||valid<4u){
            if(!c->ops.invalidate)return PORT_F11_87_BADPARAM;
            c->ops.invalidate(c->ops.context,lbn);
        }
        return header_status;
    }
    uint32_t file_num=(uint32_t)fid.number|((uint32_t)(level2?fid.number_extension:0)<<16);
    if(c->cleanup_spool){
        int spool=level2?((le32(result+52)&(1u<<12))!=0):((result[13]&16u)!=0);
        uint32_t reserved=level2?c->volume->reserved_files:5u;
        if(!spool&&file_num>reserved)return PORT_F11_87_NOSUCHFILE;
    }
    *lbn_out=lbn;
    return PORT_F11_87_NORMAL;
}
uint32_t port_f11a_read_header(struct port_f11_87_context *c,
       const struct port_f11_86_fid *id,struct port_f11_87_fcb *f,
       uint8_t h[512],uint32_t *lbn){return read_header(c,id,f,h,lbn,0);}
uint32_t port_f11b_read_header(struct port_f11_87_context *c,
       const struct port_f11_86_fid *id,struct port_f11_87_fcb *f,
       uint8_t h[512],uint32_t *lbn){return read_header(c,id,f,h,lbn,1);}

/* CREWIN: the real ACP window turner supplies the segment chain. */
static struct port_f11_87_window *create_window(struct port_f11_87_context *c,
      uint16_t access,int32_t size,const uint8_t h[512],uint32_t pid,
      struct port_f11_87_fcb *f,int v2){
    if(!c||!c->volume||!h||!f||!c->ops.turn_window)return 0;
    if(!size)size=c->volume->default_window;
    struct port_f11_87_window *w=0;
    if(size!=-1){
        if(size<PORT_F11_87_MIN_WINDOW)size=PORT_F11_87_MIN_WINDOW;
        if(size>PORT_F11_87_MAX_WINDOW)size=PORT_F11_87_MAX_WINDOW;
        if(!c->ops.alloc)return 0;
        w=c->ops.alloc(c->ops.context,sizeof(*w)+(size_t)size*6u,2u);
        if(!w)return 0;
        zero((uint8_t*)w,sizeof(*w));w->pointer_count=(uint16_t)size;
    }
    struct port_f11_87_window *primary=w;
    uint32_t status=c->ops.turn_window(c->ops.context,w,h,1u,1u,&primary);
    if(!(status&1u)||!primary){c->status=(uint16_t)status;return 0;}
    unsigned traversed=0;
    for(w=primary;w;w=w->next_segment){
        if(++traversed>65536u)__builtin_trap();
        w->pid=pid;w->ucb=c->ucb;w->access=access;w->fcb=f;
        w->read=1;w->write=w->write_access;
        if(v2){if(c->volume->current_rvn)w->rvtable=c->volume->rvtable;
            if(le32(h+52)&8u)w->read_check=1;
            if(le32(h+52)&16u)w->write_check=1;
        }else{
            if(h[12]&8u)w->read_check=1;
            if(h[12]&16u)w->write_check=1;
        }
    }
    return primary;
}
struct port_f11_87_window *port_f11a_create_window(struct port_f11_87_context *c,
      uint16_t a,int32_t n,const uint8_t h[512],uint32_t pid,
      struct port_f11_87_fcb *f){return create_window(c,a,n,h,pid,f,0);}
struct port_f11_87_window *port_f11b_create_window(struct port_f11_87_context *c,
      uint16_t a,int32_t n,const uint8_t h[512],uint32_t pid,
      struct port_f11_87_fcb *f){return create_window(c,a,n,h,pid,f,1);}

/* GETFIB: user FIB copied into fixed ACP workspace. */
static uint32_t get_fib(struct port_f11_87_context *c,const uint8_t *input,
                        size_t length,uint8_t function,
                        struct port_f11_87_fib_result *out,int level2){
    if(!c||!c->volume||!out||(!input&&length))return PORT_F11_87_BADPARAM;
    zero(out->bytes,sizeof(out->bytes));
    size_t n=length<sizeof(out->bytes)?length:sizeof(out->bytes);
    if(n)copy(out->bytes,input,n);
    out->length=(uint16_t)n;
    uint8_t *p=out->bytes;
    /* FIB$L_ACCTL, FIB$W_FID, FIB$W_DID. */
    const unsigned fid=4u,did=10u;
    if(le16(p+did)==0xffffu&&le16(p+did+2)==0xffffu&&
       (!level2||le16(p+did+4)!=0xff00u)){
        wr16(p+did,4u);wr16(p+did+2,4u);wr16(p+did+4,0u);
    }
    if(level2&&(le16(p+did)||le16(p+did+4))){
        c->cleanup_directory=1;
        if(c->volume->extended_fid){
            if(!p[did+4])p[did+4]=(uint8_t)c->volume->current_rvn;
        }else if(!le16(p+did+4))wr16(p+did+4,c->volume->current_rvn);
    }
    if(c->window){
        struct port_f11_87_fcb *f=c->window->fcb;
        if(!f)return PORT_F11_87_BADPARAM;
        if(!le16(p+fid)&&(!level2||!le16(p+fid+4))){
            wr16(p+fid,f->fid.number);wr16(p+fid+2,f->fid.sequence);
            wr16(p+fid+4,f->fid.volume);
        }
    }
    if(level2){
        if(c->volume->extended_fid){if(!p[fid+4])p[fid+4]=p[did+4];
            if(!p[fid+4])p[fid+4]=(uint8_t)c->volume->current_rvn;
        }else{
            if(!le16(p+fid+4))wr16(p+fid+4,le16(p+did+4));
            if(!le16(p+fid+4))wr16(p+fid+4,c->volume->current_rvn);
        }
    }
    if(c->window){
        struct port_f11_87_fcb *f=c->window->fcb;
        uint32_t num=(uint32_t)le16(p+fid)|(level2?(uint32_t)p[fid+5]<<16:0u);
        uint32_t fnum=f->fid.number|(level2?(uint32_t)f->fid.number_extension<<16:0u);
        if(num!=fnum||le16(p+fid+4)!=f->fid.volume){
            if(function==PORT_F11_87_IO_DEACCESS){ /* IO$_DEACCESS */
                wr16(p+fid,f->fid.number);wr16(p+fid+2,f->fid.sequence);
                wr16(p+fid+4,f->fid.volume);c->status=PORT_F11_87_BADPARAM;
            }else {c->window=0;c->primary=0;}
        }
    }else if(length<PORT_F11_87_FIB_ACCDATA&&(!level2||function!=PORT_F11_87_IO_ACPCONTROL)){
        return PORT_F11_87_BADPARAM;
    }
    if(level2)p[23]&=(uint8_t)~0x80u; /* FIB$V_NOCHARGE is bit 15 of FIB$W_EXCTL. */
    return PORT_F11_87_NORMAL;
}
uint32_t port_f11a_get_fib(struct port_f11_87_context *c,const uint8_t *in,size_t n,
                           uint8_t func,struct port_f11_87_fib_result *out){
    return get_fib(c,in,n,func,out,0);
}
uint32_t port_f11b_get_fib(struct port_f11_87_context *c,const uint8_t *in,size_t n,
                           uint8_t func,struct port_f11_87_fib_result *out){
    return get_fib(c,in,n,func,out,1);
}

/* MAKACC: keep VCB/FCB/window queues and read/write counts consistent. */
static uint32_t make_access(struct port_f11_87_context *c,struct port_f11_87_fcb *f,
                            struct port_f11_87_window *w,
                            struct port_f11_87_window **writeback,int level2){
    if(!c||!c->volume||!f||!w||!writeback)return PORT_F11_87_BADPARAM;
    struct port_f11_87_volume *v=c->volume;
    if(f->accesses==0){
        if(!f->directory)add_list(v,f);
        else if(!level2){f->directory=0;v->directory_lru++;}
    }
    struct port_f11_87_window *p=w;
    unsigned segments=0;
    while(p){
        if(++segments>65536u)__builtin_trap();
        p->next_fcb=0;
        if(f->window_tail)f->window_tail->next_fcb=p;else f->windows=p;
        f->window_tail=p;
        p=p->next_segment;
    }
    f->accesses++;
    if(w->no_read)f->exclusive=1;
    if(w->no_write)f->no_writers++;
    if(w->no_truncate)f->no_truncate++;
    if(w->write){
        if(!f->writers){
            if(f->fid.number==1u)v->write_index=1;
            if(f->fid.number==2u)v->write_bitmap=1;
            if(level2&&f->directory){
                f->dirseq++;
                for(unsigned i=0;i<sizeof(f->dirindex);i++)f->dirindex[i]=0xffu;
            }
        }
        f->writers++;
    }
    c->open_count++;c->total_opens++;v->transactions++;
    *writeback=w;c->cleanup_deaccess=1;
    return PORT_F11_87_NORMAL;
}
uint32_t port_f11a_make_access(struct port_f11_87_context *c,struct port_f11_87_fcb *f,
                               struct port_f11_87_window *w,struct port_f11_87_window **wb){
    return make_access(c,f,w,wb,0);
}
uint32_t port_f11b_make_access(struct port_f11_87_context *c,struct port_f11_87_fcb *f,
                               struct port_f11_87_window *w,struct port_f11_87_window **wb){
    return make_access(c,f,w,wb,1);
}

/* IODONE: put a real IRP on the I/O post queue, then test dismount. */
static uint32_t io_done(struct port_f11_87_context *c,struct port_f11_87_packet *p,
                         uint32_t st,uint32_t count,struct port_f11_86_abd *abd,
                         const uint8_t *fib,size_t fiblen,int level2){
    if(!c||!p||!c->volume||!c->ops.post_io||!c->ops.check_dismount)
        return PORT_F11_87_BADPARAM;
    p->status=st;p->value=count;
    uint8_t function=(uint8_t)(p->io_func&0x3fu);
    int physical=(function==PORT_F11_87_IO_READPBLK ||
                  function==PORT_F11_87_IO_WRITEPBLK);
    if(physical){
        p->flags&=(uint16_t)~PORT_F11_87_IRP_VIRTUAL;
        p->user_page=p->saved_page;
        p->byte_count=p->original_bytes;
    }else{
        struct port_f11_87_volume *v=level2&&p->volume?p->volume:c->volume;
        if(v->transactions)v->transactions--;
        if(abd && (p->flags & PORT_F11_87_IRP_COMPLX)){
            abd->name_count=0;
            if(abd->result&&fib){
                unsigned n=abd->result_count<fiblen?abd->result_count:(unsigned)fiblen;
                copy(abd->result,fib,n);
                for(unsigned i=n;i<abd->result_count;i++)abd->result[i]=0;
            }
            if(!(p->flags&PORT_F11_87_IRP_FUNC)){
                p->flags|=PORT_F11_87_IRP_FUNC;
                p->byte_count=4; /* ABD$C_ATTRIB */
            }
        }
    }
    uint32_t result=c->ops.post_io(c->ops.context,p);
    if(!(result&1u))return result;
    return c->ops.check_dismount(c->ops.context);
}
uint32_t port_f11a_io_done(struct port_f11_87_context *c,struct port_f11_87_packet *p,
       uint32_t st,uint32_t count,struct port_f11_86_abd *abd,const uint8_t *fib,size_t n){
    return io_done(c,p,st,count,abd,fib,n,0);
}
uint32_t port_f11b_io_done(struct port_f11_87_context *c,struct port_f11_87_packet *p,
       uint32_t st,uint32_t count,struct port_f11_86_abd *abd,const uint8_t *fib,size_t n){
    return io_done(c,p,st,count,abd,fib,n,1);
}
