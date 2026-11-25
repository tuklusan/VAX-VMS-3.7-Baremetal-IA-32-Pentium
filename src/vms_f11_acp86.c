#include "vms_f11_acp86.h"

static uint16_t rd16(const uint8_t *p){return (uint16_t)((uint16_t)p[0]|((uint16_t)p[1]<<8));}
static uint32_t rd32(const uint8_t *p){return (uint32_t)rd16(p)|((uint32_t)rd16(p+2)<<16);}
static void wr16(uint8_t *p,uint16_t v){p[0]=(uint8_t)v;p[1]=(uint8_t)(v>>8);}
static void put2(uint8_t *p,unsigned v){p[0]=(uint8_t)('0'+v/10u%10u);p[1]=(uint8_t)('0'+v%10u);}
static uint32_t checksum(uint8_t *p){
    uint16_t sum=0;
    if(!p)return 0;
    for(unsigned i=0;i<510;i+=2)sum=(uint16_t)(sum+rd16(p+i));
    uint32_t match=rd16(p+510)==sum;
    wr16(p+510,sum);
    return match;
}
uint32_t port_f11a_checksum(uint8_t p[512]){return checksum(p);}
uint32_t port_f11b_checksum(uint8_t p[512]){return checksum(p);}

static void copy_filled(uint8_t *out,uint16_t cap,const uint8_t *src,size_t count){
    if(!out)return;
    unsigned i=0;
    while(i<cap&&i<count){out[i]=src?src[i]:0;i++;}
    while(i<cap)out[i++]=0;
}
static uint32_t copy_name(const struct port_f11_86_abd *p){
    if(!p||(!p->name&&p->name_count)||(!p->result&&p->result_count))return 0;
    copy_filled(p->result,p->result_count,p->name,p->name_count);
    if(p->result_length)*p->result_length=p->name_count;
    return 1u;
}
uint32_t port_f11a_copy_name(const struct port_f11_86_abd *p){return copy_name(p);}
uint32_t port_f11b_copy_name(const struct port_f11_86_abd *p){return copy_name(p);}
uint32_t port_f11a_return_dir(const uint8_t *text,uint16_t count,
                              const struct port_f11_86_abd *p){
    if(!p||(!text&&count)||(!p->result&&p->result_count))return 0;
    if(p->result_length)*p->result_length=count;
    copy_filled(p->result,p->result_count,text,count);
    return 1;
}
uint32_t port_f11b_return_dir(const uint8_t *name,uint16_t len,uint16_t version,
                              const struct port_f11_86_abd *p){
    if(!p||(!name&&len)||(!p->result&&p->result_count))return 0;
    uint8_t buf[263];
    if(len>255)return 0;
    unsigned n=0;
    for(unsigned i=0;i<len;i++)buf[n++]=name[i];
    buf[n++]=';';
    uint8_t digits[6];unsigned d=0;
    int32_t signed_version=(int16_t)version;
    if(signed_version<0){buf[n++]='-';signed_version=-signed_version;}
    do{digits[d++]=(uint8_t)('0'+(unsigned)signed_version%10u);
        signed_version/=10;}while(signed_version);
    while(d)buf[n++]=digits[--d];
    if(p->result_length)*p->result_length=(uint16_t)n;
    copy_filled(p->result,p->result_count,buf,n);
    return 1;
}

/* SHRLIB/F11DEF.B32 FM2 map formats: 0 placement, 1/2/3 extents. */
uint32_t port_f11b_get_map_pointer(struct port_f11_86_map *m){
    if(!m||!m->bytes||m->offset>m->length ||m->length-m->offset<2u)return 0;
    const uint8_t *p=m->bytes+m->offset;
    unsigned f=rd16(p)>>14;
    if(f==0u){m->offset+=2u;if(m->length-m->offset<2u){m->blocks=0;m->lbn=0;return 1;}
        p=m->bytes+m->offset;f=rd16(p)>>14;
    }
    const unsigned need=f==0u?2u:f==1u?4u:f==2u?6u:8u;
    if(m->length-m->offset<need)return 0;
    m->offset+=need;
    if(f==0u){m->blocks=0;m->lbn=0;}
    else if(f==1u){m->blocks=(uint32_t)p[0]+1u;
        m->lbn=(((uint32_t)p[1]&63u)<<16)|rd16(p+2);
    }else if(f==2u){m->blocks=((uint32_t)rd16(p)&0x3fffu)+1u;m->lbn=rd32(p+2);}
    else{m->blocks=((((uint32_t)rd16(p)&0x3fffu)<<16) | rd16(p+2))+1u;
        m->lbn=rd32(p+4);}
    return 1;
}
uint32_t port_f11b_file_size(const uint8_t h[512],uint32_t *size){
    if(!h||!size||h[7]!=2u)return 0;
    unsigned offset=(unsigned)h[1]*2u,bytes=(unsigned)h[58]*2u;
    if(offset>510||bytes>510u-offset)return 0;
    struct port_f11_86_map m={h+offset,bytes,0,0,0};
    uint32_t n=0;
    while(m.offset<m.length){
        if(!port_f11b_get_map_pointer(&m)||UINT32_MAX-n<m.blocks)return 0;
        n+=m.blocks;
    }
    *size=n;
    return 1;
}
void port_f11b_apply_rvn(uint16_t *p,uint16_t current,int extended){
    if(!p)return;
    if(extended){uint16_t v=*p;uint8_t low=(uint8_t)v;
        if(!low)low=(uint8_t)current;
        if(low==1u&&!current)low=0u;
        *p=(uint16_t)((v&0xff00u)|low);
    }else{if(!*p)*p=current;if(*p==1u&&!current)*p=0;}
}
void port_f11b_default_rvn(uint16_t *p,uint16_t current,int extended){
    if(!p)return;
    if(extended){if((uint8_t)*p==(uint8_t)current)*p=(uint16_t)(*p&0xff00u);}
    else if(*p==current)*p=0;
}
uint32_t port_f11b_match_name(const uint8_t *a,size_t al,const uint8_t *p,size_t pl){
    if((!a&&al)||(!p&&pl))return 0;
    size_t ai=0,pi=0,star_ai=0,star_pi=0;
    int star=0;
    while(1){
        if(pi<pl&&p[pi]=='*'){
            ++pi;
            if(pi==pl)return 1;
            star=1;star_ai=ai;star_pi=pi;
            continue;
        }
        if(pi<pl&&ai<al&&(p[pi]==a[ai]||p[pi]=='%')){
            ++ai;++pi;continue;
        }
        if(pi==pl&&ai==al)return 1;
        if(!star||star_ai==al)return 0;
        ai=++star_ai;pi=star_pi;
    }
}
/* F11A BADSCN.B32 deliberately has no action. */
void port_f11a_scan_badlog(void){}

/* EXE$GETACCESS returns four adjacent access classes; combine their nibbles. */
static uint32_t get_access(void *ctx,port_f11_86_getaccess_fn f,uint32_t uic,uint16_t prot){
    if(!f)return 0;
    uint32_t a=f(ctx,uic,prot);
    return ((a>>12)|(a>>8)|(a>>4)|a)&15u;
}
uint32_t port_f11a_get_access(void *ctx,port_f11_86_getaccess_fn f,uint32_t uic,uint16_t p){
    return get_access(ctx,f,uic,p);
}
uint32_t port_f11b_get_access(void *ctx,port_f11_86_getaccess_fn f,uint32_t uic,uint16_t p){
    return get_access(ctx,f,uic,p);
}
static uint32_t lock_db(const struct port_f11_86_lockops *o,int unlock){
    if(!o)return 0;
    if(unlock){if(!o->unlock||!o->set_ipl_zero)return 0;
        uint32_t s=o->unlock(o->context);
        if(s&1u)o->set_ipl_zero(o->context);
        return s;
    }
    return o->lockw?o->lockw(o->context):0;
}
uint32_t port_f11a_lock_iodb(const struct port_f11_86_lockops *o){return lock_db(o,0);}
uint32_t port_f11a_unlock_iodb(const struct port_f11_86_lockops *o){return lock_db(o,1);}
uint32_t port_f11b_lock_iodb(const struct port_f11_86_lockops *o){return lock_db(o,0);}
uint32_t port_f11b_unlock_iodb(const struct port_f11_86_lockops *o){return lock_db(o,1);}

static uint32_t requeue(struct port_f11_86_request *rq,void *ucb,uint32_t lbn,
                        uint32_t unmapped,const struct port_f11_86_iops *o,int level2){
    if(!rq||!o||!o->logical_to_physical||!o->insert_ioq)return 0;
    if(level2)rq->ucb=ucb;
    if(unmapped){
        uint32_t bytes=unmapped<<9u;
        uint16_t n=(uint16_t)((uint32_t)rq->bytes-bytes);
        n=(uint16_t)(n+511u);
        rq->bytes=(uint16_t)(n&~511u);
    }
    uint32_t physical=0;
    uint32_t rc=o->logical_to_physical(o->context,ucb,lbn,&physical,&rq->func);
    if(!(rc&1u))return rc;
    rq->lbn=physical;
    return o->insert_ioq(o->context,rq);
}
uint32_t port_f11a_requeue(struct port_f11_86_request *r,void *u,uint32_t l,
                          uint32_t n,const struct port_f11_86_iops *o){return requeue(r,u,l,n,o,0);}
uint32_t port_f11b_requeue(struct port_f11_86_request *r,void *u,uint32_t l,
                          uint32_t n,const struct port_f11_86_iops *o){return requeue(r,u,l,n,o,1);}
static int64_t map_window(void *window,void *original_ucb,uint32_t vbn,
                          uint32_t count,uint32_t *unmapped,void **out_ucb,
                          const struct port_f11_86_mapops *o){
    if(!unmapped)return -1;
    if(!o||!o->map_vblock||!vbn||!count||count>UINT32_MAX/512u){
        *unmapped=count;
        return -1;
    }
    void *ucb=original_ucb;
    uint32_t lbn=0,remaining=0;
    const uint32_t requested=count*512u;
    uint32_t rc=o->map_vblock(o->context,window,vbn,requested,
                              &ucb,&lbn,&remaining);
    if(out_ucb)*out_ucb=ucb;
    if(!(rc&1u)||lbn==UINT32_MAX||remaining>requested||(remaining&511u)){
        *unmapped=count;
        return -1;
    }
    *unmapped=remaining/512u;
    return (int64_t)lbn;
}
int64_t port_f11a_map_window(void *w,void *u,uint32_t vbn,uint32_t n,
                            uint32_t *left,const struct port_f11_86_mapops *o){
    return map_window(w,u,vbn,n,left,0,o);
}
int64_t port_f11b_map_window(void *w,void *u,uint32_t vbn,uint32_t n,
                            uint32_t *left,void **result,
                            const struct port_f11_86_mapops *o){
    return map_window(w,u,vbn,n,left,result,o);
}

static uint32_t next_header(const uint8_t *old,void *fcb,
                             const struct port_f11_86_fid *specified,
                             const uint16_t *segnum,int extended,
                             const struct port_f11_86_headerops *o,
                             uint8_t out[512],int *found,int level2){
    if(!o||!o->read_header||!out||!found)return 0;
    *found=0;
    struct port_f11_86_fid fid={0};
    uint16_t seg=0;
    if(specified&&segnum){fid=*specified;seg=*segnum;}
    else if(level2){
        if(!old)return 0;
        fid.number=rd16(old+14u);fid.sequence=rd16(old+16u);
        fid.volume=extended?old[18u]:rd16(old+18u);
        fid.number_extension=old[19u];
        seg=(uint16_t)(rd16(old+4u)+1u);
    }else{
        if(!old)return 0;
        unsigned pos=(unsigned)old[1]*2u;
        if(pos>506u)return PORT_F11_86_BADFILEHDR;
        fid.number=rd16(old+pos+2u);fid.sequence=rd16(old+pos+4u);
        seg=(uint8_t)(old[pos]+1u);
    }
    if(!level2)seg=(uint8_t)seg;
    if(!fid.number&&(!level2||!extended||!fid.number_extension))return 1;
    void *extension_fcb=fcb?((const struct port_f11_86_file_control *)fcb)->extension_fcb:0;
    uint32_t rc=o->read_header(o->context,fid,extension_fcb,out);
    if(!(rc&1u))return rc;
    if(level2){if(rd16(out+4u)!=seg)return PORT_F11_86_BADFILEHDR;}
    else{
        unsigned at=(unsigned)out[1]*2u;
        if(at>508u||out[at]!=seg)return PORT_F11_86_BADFILEHDR;
    }
    *found=1;
    return 1;
}
uint32_t port_f11a_next_header(const uint8_t *h,void *f,
                               const struct port_f11_86_fid *id,const uint16_t *n,
                               const struct port_f11_86_headerops *o,
                               uint8_t result[512],int *found){
    return next_header(h,f,id,n,0,o,result,found,0);
}
uint32_t port_f11b_next_header(const uint8_t *h,void *f,
                               const struct port_f11_86_fid *id,const uint16_t *n,
                               int ext,const struct port_f11_86_headerops *o,
                               uint8_t result[512],int *found){
    return next_header(h,f,id,n,ext,o,result,found,1);
}

/* 100 ns ticks since 17-Nov-1858, converted by the same Gregorian rules as
 * VMS SYS$NUMTIM. No host clock is read unless a caller supplies one. */
static uint64_t divide64(uint64_t value,uint64_t divisor,uint64_t *remainder){
    uint64_t q=0,rem=0;
    for(int bit=63;bit>=0;bit--){
        rem=(rem<<1)|(value>>bit&1u);
        if(rem>=divisor){rem-=divisor;q|=UINT64_C(1)<<bit;}
    }
    if(remainder)*remainder=rem;
    return q;
}
static uint32_t get_time(uint8_t output[13],const uint64_t *provided,
                         const struct port_f11_86_timeops *o,int ods1){
    if(!output)return 0;
    uint64_t ticks=0;
    if(provided){ticks=*provided;if(!ticks&&ods1){for(unsigned i=0;i<13;i++)output[i]=0;return 1;}}
    else if(!o||!o->now||!(o->now(o->context,&ticks)&1u))return 0;
    uint64_t rem=0,days=divide64(ticks,UINT64_C(864000000000),&rem);
    if(days>3000000u)return 0;
    uint32_t secs=(uint32_t)divide64(rem,UINT64_C(10000000),0);
    uint32_t h=(uint32_t)(secs/3600u),min=(uint32_t)(secs%3600u/60u),sec=(uint32_t)(secs%60u);
    /* Gregorian civil date from a day number relative to 1970-01-01. */
    int32_t z=(int32_t)days-40587+719468;
    int32_t era=z>=0?z/146097:(z-146096)/146097;
    unsigned doe=(unsigned)(z-era*146097);
    unsigned yoe=(doe-doe/1460+doe/36524-doe/146096)/365;
    int32_t year=(int32_t)yoe+era*400;
    unsigned doy=doe-(365u*yoe+yoe/4u-yoe/100u);
    unsigned mp=(5u*doy+2u)/153u;
    unsigned d=doy-(153u*mp+2u)/5u+1u;
    unsigned m=mp+(mp<10u?3u:(unsigned)-9);
    year+=(m<=2u);
    if(year<0||year>9999||m<1||m>12||d>31)return 0;
    static const char months[]="JANFEBMARAPRMAYJUNJULAUGSEPOCTNOVDEC";
    put2(output,d);
    for(unsigned i=0;i<3;i++)output[2+i]=(uint8_t)months[(m-1u)*3u+i];
    put2(output+5,(unsigned)year%100u);
    put2(output+7,h);put2(output+9,min);put2(output+11,sec);
    return 1;
}
uint32_t port_f11a_get_time(uint8_t o[13],const uint64_t *t,
                            const struct port_f11_86_timeops *src){return get_time(o,t,src,1);}
uint32_t port_f11b_get_time(uint8_t o[13],const uint64_t *t,
                            const struct port_f11_86_timeops *src){return get_time(o,t,src,0);}
