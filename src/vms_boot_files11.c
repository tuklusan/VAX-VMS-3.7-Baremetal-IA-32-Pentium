#include "vms_boot_files11.h"
static uint16_t get16(const uint8_t *p) {
    return (uint16_t)(p[0] | (uint16_t)(p[1]<<8));
}
static uint32_t get32(const uint8_t *p) {
    return (uint32_t)p[0] | ((uint32_t)p[1]<<8) |
        ((uint32_t)p[2]<<16) | ((uint32_t)p[3]<<24);
}
/* MOUNT/SRC/CHKSM2.B32: additive 16-bit sums preceding each cell. */
static int checked_sum(const uint8_t *p,uint32_t checksum_byte) {
    uint16_t sum=0;
    for(uint32_t i=0;i<checksum_byte;i+=2u)
        sum=(uint16_t)(sum+get16(p+i));
    return sum==get16(p+checksum_byte);
}
uint32_t port_f11_home_read(const uint8_t *p,size_t bytes,
                            uint32_t lbn,struct port_f11_home *out) {
    if(!p || bytes<512u || !out) return PORT_F11_HOME_BADARG;
    /* HM2$W_STRUCLEV = 512 (VMS 3.7 Files-11 structure level 2). */
    if(p[13u]!=2u) return PORT_F11_HOME_NOT_ODS2;
    out->home_lbn=get32(p);
    out->alternate_home_lbn=get32(p+4u);
    out->alternate_index_lbn=get32(p+8u);
    out->cluster=get16(p+14u);
    out->index_bitmap_lbn=get32(p+24u);
    out->max_files=get32(p+28u);
    out->index_bitmap_blocks=get16(p+32u);
    out->reserved_files=get16(p+34u);
    if(out->home_lbn != lbn || !out->alternate_index_lbn ||
       !get16(p+16u) || !get16(p+18u) || !get16(p+20u) ||
       !get16(p+22u) || !out->index_bitmap_lbn || !out->cluster ||
       !out->index_bitmap_blocks || !out->max_files ||
       !out->reserved_files || out->max_files<out->reserved_files)
        return PORT_F11_HOME_BAD_FIELDS;
    if(!checked_sum(p,58u) || !checked_sum(p,510u))
        return PORT_F11_HOME_BAD_CHECKSUM;
    return PORT_F11_HOME_OK;
}

uint32_t port_f11_mount(const struct boo_io_driver *driver,
                        uint32_t read_function,
                        struct port_f11_volume *volume) {
    if(!driver || !volume) return PORT_F11_HOME_BADARG;
    uint32_t rc=boo_rdwr_lbn(driver,1u,volume->index_header,
                             read_function,512u);
    if(!(rc&1u)) return PORT_F11_MOUNT_DISK_ERROR;
    rc=port_f11_home_read(volume->index_header,512u,1u,&volume->home);
    if(rc!=PORT_F11_HOME_OK) return rc;
    uint32_t lbn=volume->home.index_bitmap_lbn;
    uint32_t count=volume->home.index_bitmap_blocks;
    if(lbn+count<lbn) return PORT_F11_MOUNT_BAD_INDEX;
    volume->index_header_lbn=lbn+count;
    rc=boo_rdwr_lbn(driver,volume->index_header_lbn,
                    volume->index_header,read_function,512u);
    if(!(rc&1u)) return PORT_F11_MOUNT_DISK_ERROR;
    const uint8_t *h=volume->index_header;
    /* FH2$B_IDOFFSET, FH2$B_MPOFFSET, FH2$W_STRUCLEV,
     * FH2$W_FID_NUM/SEQ/RVN and FH2$W_CHECKSUM, from F11DEF.B32. */
    if(h[7u]!=2u || get16(h+8u)!=1u ||
       get16(h+10u)!=1u || get16(h+12u)!=0u ||
       h[0u]==0u || h[1u]<=h[0u] || h[1u]>=255u ||
       !checked_sum(h,510u)) return PORT_F11_MOUNT_BAD_INDEX;
    /* FIL$MOUNT stores the index bitmap length plus the reserved
     * initial file-header VBNs, 4*HM2$W_CLUSTER. */
    volume->index_vbn_offset=count+4u*volume->home.cluster;
    return PORT_F11_HOME_OK;
}

/* FIL$READVBN and GETRTRVPTR from SYS/SRC/FILEREAD.MAR, level 2.
 * VBNs are one-origin; the block map counts from the first mapped VBN. */
static uint32_t read_pointer(const uint8_t *p,unsigned limit,
                             unsigned *used,uint32_t *count,uint32_t *lbn) {
    if(limit<2u) return PORT_F11_FILE_BAD_MAP;
    uint16_t first=get16(p);
    unsigned format=first>>14;
    *lbn=0u;*count=0u;
    if(format==0u){*used=2u;return PORT_F11_HOME_OK;}
    if(format==1u){
        if(limit<4u)return PORT_F11_FILE_BAD_MAP;
        *count=(first&0xffu)+1u;
        *lbn=((uint32_t)(first>>8&0x3fu)<<16)|get16(p+2u);
        *used=4u;
    }else if(format==2u){
        if(limit<6u)return PORT_F11_FILE_BAD_MAP;
        *count=(first&0x3fffu)+1u;
        *lbn=get32(p+2u);
        *used=6u;
    }else{
        if(limit<8u)return PORT_F11_FILE_BAD_MAP;
        *count=(((uint32_t)first&0x3fffu)<<16 | get16(p+2u))+1u;
        *lbn=get32(p+4u);
        *used=8u;
    }
    if(*lbn==0xffffffffu || *count==0u || *lbn+*count<*lbn)
        return PORT_F11_FILE_BAD_MAP;
    return PORT_F11_HOME_OK;
}
uint32_t port_f11_collect_extents(const uint8_t header[512],
                                  struct port_f11_extent *out,
                                  uint32_t cap,uint32_t *count) {
    if(!header || !out || !count || !cap || header[7u]!=2u)
        return PORT_F11_HOME_BADARG;
    unsigned start=(unsigned)header[1u]*2u;
    unsigned words=header[58u];
    if(start<2u || start>=510u || !words ||
       words*2u>510u-start)return PORT_F11_FILE_BAD_MAP;
    unsigned pos=start,end=start+words*2u;
    uint32_t used=0u;
    while(pos<end) {
        uint32_t len=0,lbn=0;unsigned length=0;
        uint32_t status=read_pointer(header+pos,end-pos,&length,&len,&lbn);
        if(status!=PORT_F11_HOME_OK)return status;
        pos+=length;
        if(!len)continue;
        if(used>=cap)return PORT_F11_FILE_BAD_MAP;
        out[used].blocks=len;
        out[used].lbn=lbn;
        used++;
    }
    if(!used)return PORT_F11_FILE_BAD_MAP;
    *count=used;
    return PORT_F11_HOME_OK;
}
uint32_t port_f11_collect_file_extents(const struct boo_io_driver *disk,
                                uint32_t fn,const struct port_f11_volume *vol,
                                struct port_f11_fid fid,
                                struct port_f11_extent *out,
                                uint32_t cap,uint32_t *used,
                                uint32_t *blocks,uint32_t *start_lbn) {
    if(!disk||!vol||!fid.number||!out||!cap||!used||!blocks||!start_lbn)
        return PORT_F11_HOME_BADARG;
    struct port_f11_fid seen[128];
    uint32_t seen_count=0u,total=0u,count=0u;
    uint32_t first=0u,last=0u;
    int contiguous=1;
    while(fid.number){
        if(seen_count>=128u)return PORT_F11_FILE_BAD_MAP;
        for(uint32_t i=0;i<seen_count;i++)
            if(seen[i].number==fid.number &&
               seen[i].sequence==fid.sequence &&
               seen[i].volume==fid.volume)return PORT_F11_FILE_BAD_MAP;
        seen[seen_count++]=fid;
        uint8_t h[512];
        uint32_t rc=port_f11_read_file_header(disk,fn,vol,fid,h);
        if(rc!=PORT_F11_HOME_OK)return rc;
        if(!h[58u])return PORT_F11_FILE_BAD_MAP;
        uint32_t n=0u;
        rc=port_f11_collect_extents(h,out+count,cap-count,&n);
        if(rc!=PORT_F11_HOME_OK)return rc;
        for(uint32_t i=0;i<n;i++){
            const uint32_t at=out[count+i].lbn,len=out[count+i].blocks;
            if(UINT32_MAX-total<len)return PORT_F11_FILE_BAD_MAP;
            if(!total)first=at;
            else if(at!=last)contiguous=0;
            last=at+len;
            total+=len;
        }
        count+=n;
        fid.number=get16(h+14u);
        fid.sequence=get16(h+16u);
        fid.volume=get16(h+18u);
        if(!fid.number && (fid.sequence || fid.volume))
            return PORT_F11_FILE_BAD_MAP;
    }
    *used=count;
    *blocks=total;
    *start_lbn=contiguous?first:0u;
    return PORT_F11_HOME_OK;
}
static uint32_t mapped_data(const uint8_t h[512],uint32_t vbn,
                             uint32_t *lbn,uint32_t *total,
                             uint32_t *first,uint32_t *last) {
    if(!h||h[7u]!=2u||!h[1u] || !h[58u])
        return PORT_F11_FILE_BAD_MAP;
    unsigned map_offset=(unsigned)h[1u]*2u;
    unsigned used_words=h[58u];
    if(map_offset >= 510u || used_words>255u ||
       used_words*2u > 510u-map_offset)
        return PORT_F11_FILE_BAD_MAP;
    unsigned pos=map_offset,end=map_offset+used_words*2u;
    uint32_t covered=0u,first_lbn=0u,last_end=0u;
    int contiguous=1,have_first=0;
    while(pos<end){
        uint32_t len=0,at=0;unsigned bytes=0;
        uint32_t rc=read_pointer(h+pos,end-pos,&bytes,&len,&at);
        if(rc!=PORT_F11_HOME_OK)return rc;
        pos+=bytes;
        if(!len)continue;
        if(!have_first){first_lbn=at;have_first=1;}
        else if(last_end!=at)contiguous=0;
        last_end=at+len;
        if(vbn && vbn-1u>=covered && vbn-1u-covered<len){
            if(lbn)*lbn=at+(vbn-1u-covered);
        }
        if(UINT32_MAX-covered<len)return PORT_F11_FILE_BAD_MAP;
        covered+=len;
    }
    if(total)*total=covered;
    if(first)*first=contiguous?first_lbn:0u;
    if(last)*last=last_end;
    return (!vbn||vbn<=covered)?PORT_F11_HOME_OK:PORT_F11_FILE_EOF;
}
uint32_t port_f11_vbn_to_lbn(const uint8_t header[512],
                             uint32_t vbn,uint32_t *lbn) {
    if(!header||!lbn||!vbn)return PORT_F11_HOME_BADARG;
    return mapped_data(header,vbn,lbn,0,0,0);
}
uint32_t port_f11_read_vbn(const struct boo_io_driver *driver,
                           uint32_t read_function,const uint8_t header[512],
                           uint32_t vbn,uint8_t block[512]) {
    if(!driver||!header||!block)return PORT_F11_HOME_BADARG;
    uint32_t lbn=0;
    uint32_t rc=port_f11_vbn_to_lbn(header,vbn,&lbn);
    if(rc!=PORT_F11_HOME_OK)return rc;
    return (boo_rdwr_lbn(driver,lbn,block,read_function,512u)&1u)?
        PORT_F11_HOME_OK:PORT_F11_MOUNT_DISK_ERROR;
}
uint32_t port_f11_file_extent_stats(const uint8_t header[512],
                                   uint32_t *first_lbn,uint32_t *blocks){
    if(!header||!first_lbn||!blocks)return PORT_F11_HOME_BADARG;
    return mapped_data(header,0,0,blocks,first_lbn,0);
}
uint32_t port_f11_read_file_header(const struct boo_io_driver *driver,
                                   uint32_t read_function,
                                   const struct port_f11_volume *volume,
                                   struct port_f11_fid fid,
                                   uint8_t header[512]) {
    if(!driver||!volume||!header||!fid.number)return PORT_F11_HOME_BADARG;
    uint32_t vbn=volume->index_vbn_offset+(uint32_t)fid.number;
    if(vbn<fid.number)return PORT_F11_FILE_BAD_MAP;
    uint32_t rc=port_f11_read_vbn(driver,read_function,
                                  volume->index_header,vbn,header);
    if(rc!=PORT_F11_HOME_OK)return rc;
    if(header[7u]!=2u || get16(header+8u)!=fid.number ||
       get16(header+10u)!=fid.sequence ||
       get16(header+12u)!=fid.volume)
        return PORT_F11_FILE_BAD_SEQ;
    return checked_sum(header,510u)?PORT_F11_HOME_OK:PORT_F11_FILE_BAD_HEADER;
}

uint32_t port_f11_find_file(const struct boo_io_driver *driver,
                            uint32_t read_function,
                            const struct port_f11_volume *volume,
                            struct port_f11_fid directory,
                            const char *name_with_type,
                            uint16_t version,
                            struct port_f11_fid *found) {
    if(!driver||!volume||!name_with_type||!found)
        return PORT_F11_HOME_BADARG;
    unsigned name_len=0;
    while(name_with_type[name_len] && name_len<80u) name_len++;
    if(!name_len||name_with_type[name_len])return PORT_F11_HOME_BADARG;
    uint8_t h[512],b[512];
    uint32_t rc=port_f11_read_file_header(driver,read_function,
                                          volume,directory,h);
    if(rc!=PORT_F11_HOME_OK)return rc;
    /* FILECHAR bit 13 and RMS variable-length no-span directory records. */
    if(!(get32(h+52u)&(1u<<13)) || h[20u]!=2u || h[21u]!=8u)
        return PORT_F11_FILE_BAD_DIR;
    uint32_t first_lbn=0,mapped=0;
    rc=port_f11_file_extent_stats(h,&first_lbn,&mapped);
    if(rc!=PORT_F11_HOME_OK || !first_lbn || !mapped)
        return PORT_F11_FILE_BAD_DIR;
    /* FAT$L_EFBLK uses the original 16-bit-word-swapped VBN. */
    uint32_t eof=get32(h+28u);
    eof=(eof<<16u)|(eof>>16u);
    if(!get16(h+32u))eof--;
    if(!eof || eof>mapped)return PORT_F11_FILE_BAD_DIR;
    for(uint32_t block=0;block<eof;block++){
        if(!(boo_rdwr_lbn(driver,first_lbn+block,b,
                          read_function,512u)&1u))
            return PORT_F11_MOUNT_DISK_ERROR;
        unsigned pos=0;
        while(pos+2u<=512u){
            uint16_t size=get16(b+pos);
            if(size==0xffffu)break;
            if(size<12u || size>510u-pos || pos+size+2u>512u)
                return PORT_F11_FILE_BAD_DIR;
            unsigned namecount=b[pos+5u];
            unsigned namepad=(namecount+1u)&~1u;
            unsigned entry=pos+6u+namepad;
            unsigned limit=pos+2u+size;
            if(!namecount || entry+8u>limit)
                return PORT_F11_FILE_BAD_DIR;
            if(namecount==name_len){
                int same=1;
                for(unsigned i=0;i<namecount;i++){
                    char want=name_with_type[i],actual=(char)b[pos+6u+i];
                    if(want>='a'&&want<='z')want=(char)(want-'a'+'A');
                    if(actual>='a'&&actual<='z')actual=(char)(actual-'a'+'A');
                    if(want!=actual){same=0;break;}
                }
                if(same){
                    for(;entry+8u<=limit;entry+=8u){
                        uint16_t v=get16(b+entry);
                        if(!version || v==version){
                            found->number=get16(b+entry+2u);
                            found->sequence=get16(b+entry+4u);
                            found->volume=get16(b+entry+6u);
                            if(!found->number||!found->sequence)
                                return PORT_F11_FILE_BAD_DIR;
                            return PORT_F11_HOME_OK;
                        }
                    }
                }
            }
            pos=limit;
        }
    }
    return PORT_F11_FILE_NOT_FOUND;
}

uint32_t port_f11_open_named_file(const struct boo_io_driver *driver,
                                  uint32_t read_function,
                                  const struct port_f11_volume *volume,
                                  const char *filespec,
                                  struct port_f11_fid *fid,
                                  uint8_t header[512],
                                  uint32_t *first_lbn,
                                  uint32_t *blocks) {
    if(!driver||!volume||!filespec||!fid||!header||!first_lbn||!blocks)
        return PORT_F11_HOME_BADARG;
    /* FIL$OPENFILE strips the device and uses [SYSEXE] when no directory
     * is written. Named directories are resolved from FID$C_MFD. */
    unsigned length=0;
    while(filespec[length] && length<160u)length++;
    if(!length||filespec[length])return PORT_F11_HOME_BADARG;
    unsigned begin=0;
    for(unsigned i=0;i<length;i++)if(filespec[i]==':')begin=i+1u;
    const char *s=filespec+begin;
    unsigned remaining=length-begin;
    if(!remaining)return PORT_F11_HOME_BADARG;
    char dirname[96],filename[96];
    unsigned dirlen=0;
    const char *directories="SYSEXE";
    if(s[0]=='['||s[0]=='<'){
        char close=s[0]=='['?']':'>';
        unsigned i=1;
        while(i<remaining&&s[i]!=close)i++;
        if(i==remaining||i<2||i>=sizeof dirname)return PORT_F11_HOME_BADARG;
        for(unsigned j=1;j<i;j++)dirname[j-1]=s[j];
        dirname[i-1]=0;
        dirlen=i-1;
        directories=dirname;
        s+=i+1u;remaining-=i+1u;
    }
    if(!remaining)return PORT_F11_HOME_BADARG;
    struct port_f11_fid current={4u,4u,0u};
    unsigned d=0;
    while(d< (dirlen?dirlen:6u)) {
        char dir[96];unsigned n=0;
        while(d<(dirlen?dirlen:6u) && directories[d]!='.') {
            if(n+6u>=sizeof dir)return PORT_F11_HOME_BADARG;
            char ch=directories[d++];
            if(ch>='a'&&ch<='z')ch=(char)(ch-'a'+'A');
            if(!((ch>='A'&&ch<='Z')||(ch>='0'&&ch<='9')||
                 ch=='_'||ch=='$'))return PORT_F11_HOME_BADARG;
            dir[n++]=ch;
        }
        d++;
        if(!n)return PORT_F11_HOME_BADARG;
        dir[n++]='.';dir[n++]='D';dir[n++]='I';dir[n++]='R';dir[n]=0;
        struct port_f11_fid next;
        uint32_t rc=port_f11_find_file(driver,read_function,volume,
                                       current,dir,1u,&next);
        if(rc!=PORT_F11_HOME_OK)return rc;
        current=next;
    }
    unsigned n=0;
    while(n<remaining && s[n]!=';' && n<sizeof filename-1u){
        filename[n]=s[n];n++;
    }
    if(!n || n>=sizeof filename-1u)return PORT_F11_HOME_BADARG;
    filename[n]=0;
    uint16_t version=0;
    if(n<remaining){
        if(s[n]!=';' || n+1u==remaining)return PORT_F11_HOME_BADARG;
        uint32_t v=0;
        for(unsigned i=n+1u;i<remaining;i++){
            if(s[i]<'0'||s[i]>'9')return PORT_F11_HOME_BADARG;
            v=v*10u+(unsigned)(s[i]-'0');
            if(v>65535u)return PORT_F11_HOME_BADARG;
        }
        version=(uint16_t)v;
    }
    uint32_t rc=port_f11_find_file(driver,read_function,volume,
                                   current,filename,version,fid);
    if(rc!=PORT_F11_HOME_OK)return rc;
    rc=port_f11_read_file_header(driver,read_function,volume,*fid,header);
    if(rc!=PORT_F11_HOME_OK)return rc;
    return port_f11_file_extent_stats(header,first_lbn,blocks);
}
