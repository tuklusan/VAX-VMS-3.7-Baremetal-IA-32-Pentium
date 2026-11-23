#include "vms_validate.h"
#include "vms_uaf_v37.h"

static void bytes_copy(uint8_t *dest, const uint8_t *src, unsigned size) {
    for (unsigned i=0; i<size; ++i) dest[i]=src[i];
}
static int bytes_equal(const uint8_t *a, const uint8_t *b, unsigned size) {
    unsigned different=0;
    for (unsigned i=0; i<size; ++i) different|=(unsigned)(a[i]^b[i]);
    return different==0;
}
static void uppercase(uint8_t *bytes, unsigned size) {
    for (unsigned i=0; i<size; ++i)
        if (bytes[i]>='a' && bytes[i]<='z') bytes[i]=(uint8_t)(bytes[i]-('a'-'A'));
}
static void name_key(uint8_t key[PORT_UAF_KEY_BYTES],
                     const uint8_t *username, uint16_t len) {
    for (unsigned i=0; i<PORT_UAF_KEY_BYTES; ++i)
        key[i]=i<len?username[i]:' ';
    uppercase(key,PORT_UAF_KEY_BYTES);
}
static int locked_get(struct port_uaf_service *s,
                      const uint8_t key[PORT_UAF_KEY_BYTES],
                      void *out, uint16_t capacity) {
    for (unsigned attempt=0; attempt<=PORT_UAF_RETRY_COUNT; ++attempt) {
        int status=s->ops.get(s->context,key,out,capacity);
        if (status != PORT_UAF_RLK || attempt==PORT_UAF_RETRY_COUNT)
            return status;
        if (s->ops.delay_ms) s->ops.delay_ms(s->context,PORT_UAF_RETRY_MS);
    }
    return PORT_UAF_RLK;
}
static int validate_password(const struct port_uaf_record *record,
                             const uint8_t *password, uint16_t password_len) {
    uint8_t folded[PORT_UAF_MAX_PASSWORD];
    unsigned len=password_len;
    if (len>PORT_UAF_MAX_PASSWORD) len=PORT_UAF_MAX_PASSWORD;
    for (unsigned i=0; i<len; ++i) folded[i]=password[i];
    uppercase(folded,len);
    uint16_t salt=(uint16_t)(record->salt_lo | ((uint16_t)record->salt_hi<<8));
    int match=port_hpwd_matches(record->hash,folded,(uint16_t)len,
                 record->algorithm,salt,record->username,PORT_UAF_KEY_BYTES);
    for (unsigned i=0; i<len; ++i) folded[i]=0;
    return match?PORT_UAF_NORMAL:PORT_UAF_INVPWD;
}
static int do_search(struct port_uaf_service *s,
                     const uint8_t *username, uint16_t username_len,
                     const uint8_t *password, uint16_t password_len,
                     void *record, uint16_t capacity, int check_password) {
    if (!s || !s->ops.open || !s->ops.connect || !s->ops.get ||
        !s->ops.disconnect || !s->ops.close ||
        (!username && username_len) || (!password && password_len && check_password) ||
        !record || capacity<PORT_UAF_RECORD_BYTES)
        return PORT_UAF_BAD_ARG;
    uint8_t key[PORT_UAF_KEY_BYTES], default_key[PORT_UAF_KEY_BYTES];
    static const uint8_t default_name[7]={'D','E','F','A','U','L','T'};
    name_key(key,username,username_len);
    name_key(default_key,default_name,7);

    /* Restore exactly the caller's privilege state on all exit paths. */
    uint64_t saved=s->privileges;
    s->privileges|=s->allowed_privileges;
    int connected=0,opened=0;
    int status=s->ops.open(s->context,1,1);
    if (status==PORT_UAF_SNE || status==PORT_UAF_SPE || status==PORT_UAF_DME)
        status=s->ops.open(s->context,0,1);
    if (status!=PORT_UAF_NORMAL) goto done;
    opened=1;
    status=s->ops.connect(s->context);
    if (status!=PORT_UAF_NORMAL) goto done;
    connected=1;

    /* The literal DEFAULT account cannot log in, but its record supplies
     * the same fallback result returned for an unknown username. */
    if (bytes_equal(key,default_key,PORT_UAF_KEY_BYTES))
        status=PORT_UAF_RNF;
    else
        status=locked_get(s,key,record,capacity);
    if (status==PORT_UAF_RNF) {
        status=locked_get(s,default_key,record,capacity);
        if (status==PORT_UAF_NORMAL) status=PORT_UAF_INVUSR;
    } else if (status==PORT_UAF_NORMAL && check_password) {
        status=validate_password((const struct port_uaf_record*)record,
                                 password,password_len);
    }
done:
    if (connected) s->ops.disconnect(s->context);
    if (opened) s->ops.close(s->context);
    s->privileges=saved;
    return status;
}
int port_uaf_searchuser(struct port_uaf_service *service,
                        const uint8_t *username, uint16_t username_len,
                        void *record, uint16_t capacity) {
    return do_search(service,username,username_len,0,0,record,capacity,0);
}
int port_uaf_validate(struct port_uaf_service *service,
                      const uint8_t *username, uint16_t username_len,
                      const uint8_t *password, uint16_t password_len,
                      void *record, uint16_t capacity) {
    return do_search(service,username,username_len,password,password_len,
                     record,capacity,1);
}

static uint8_t disk_byte(const struct port_uaf_disk *disk,unsigned offset) {
    return (uint8_t)((disk->directory[offset/2]>>(8*(offset&1u)))&0xffu);
}
static int disk_open(void *context,int shared,int system_names_only) {
    struct port_uaf_disk *disk=context;
    (void)shared;
    if (!system_names_only || !disk || !disk->read) return PORT_UAF_BAD_ARG;
    disk->opened=0;
    if (disk->read(disk->context,disk->index_lba,disk->directory)) return PORT_UAF_IO;
    static const uint8_t magic[8]={'V','3','7','U','A','F','I','X'};
    for (unsigned i=0; i<8; ++i)
        if (disk_byte(disk,i)!=magic[i]) return PORT_UAF_IO;
    if ((disk_byte(disk,8)!=1u && disk_byte(disk,8)!=2u) ||
        disk_byte(disk,9)>31u) return PORT_UAF_IO;
    disk->format_version=disk_byte(disk,8);
    disk->opened=1;
    return PORT_UAF_NORMAL;
}
static int disk_connect(void *context) {
    struct port_uaf_disk *disk=context;
    if (!disk || !disk->opened) return PORT_UAF_IO;
    disk->connected=1;
    return PORT_UAF_NORMAL;
}
static int disk_get(void *context,const uint8_t key[PORT_UAF_KEY_BYTES],
                    void *result,uint16_t size) {
    struct port_uaf_disk *disk=context;
    if (!disk || !disk->connected || size<PORT_UAF_RECORD_BYTES)
        return PORT_UAF_SMALL;
    unsigned count=disk_byte(disk,9);
    for (unsigned i=0; i<count; ++i) {
        unsigned offset=16u+i*16u;
        int compare=0;
        for (unsigned j=0; j<PORT_UAF_KEY_BYTES; ++j) {
            if (disk_byte(disk,offset+j)!=key[j]) {compare=1;break;}
        }
        if (compare) continue;
        uint32_t lba=0;
        for (unsigned j=0; j<4; ++j)
            lba |= (uint32_t)disk_byte(disk,offset+12u+j) << (j*8u);
        if (lba<=disk->index_lba || lba>=disk->index_lba+4096u)
            return PORT_UAF_IO;
        uint16_t sector[256];
        if (disk->read(disk->context,lba,sector)) return PORT_UAF_IO;
        uint8_t data[PORT_UAF_RECORD_BYTES];
        for (unsigned j=0; j<256; ++j) {
            data[j*2]=(uint8_t)sector[j];
            data[j*2+1]=(uint8_t)(sector[j]>>8);
        }
        if (!bytes_equal(data,key,PORT_UAF_KEY_BYTES)) return PORT_UAF_IO;
        if (disk->format_version==2u) {
            if (!vms37_uaf_decode((struct port_uaf_record*)result,
                                  data,PORT_UAF_RECORD_BYTES))
                return PORT_UAF_IO;
        } else {
            bytes_copy(result,data,PORT_UAF_RECORD_BYTES);
        }
        return PORT_UAF_NORMAL;
    }
    return PORT_UAF_RNF;
}
static void disk_disconnect(void *context) {
    ((struct port_uaf_disk*)context)->connected=0;
}
static void disk_close(void *context) {
    ((struct port_uaf_disk*)context)->opened=0;
}
void port_uaf_disk_init(struct port_uaf_disk *disk,port_uaf_read_sector read,
                        void *context,uint32_t index_lba) {
    if (!disk) return;
    disk->read=read;
    disk->context=context;
    disk->index_lba=index_lba;
    disk->opened=0;
    disk->connected=0;
    disk->format_version=0;
    for (unsigned i=0; i<256; ++i) disk->directory[i]=0;
}
struct port_uaf_service port_uaf_disk_service(struct port_uaf_disk *disk) {
    struct port_uaf_service service={0};
    service.context=disk;
    service.ops.open=disk_open;
    service.ops.connect=disk_connect;
    service.ops.get=disk_get;
    service.ops.disconnect=disk_disconnect;
    service.ops.close=disk_close;
    return service;
}

uint32_t port_uaf_guest_selftest(port_uaf_read_sector read,void *context) {
    struct port_uaf_disk disk;
    port_uaf_disk_init(&disk,read,context,3);
    struct port_uaf_service service=port_uaf_disk_service(&disk);
    service.privileges=UINT64_C(0x123);
    service.allowed_privileges=UINT64_C(0xfff000);
    struct port_uaf_record record;
    uint32_t failed=0;
    static const uint8_t labuser[]="labuser";
    static const uint8_t password[]="password";
    if (port_uaf_validate(&service,labuser,7,password,8,
                           &record,sizeof record)!=PORT_UAF_NORMAL ||
        !bytes_equal(record.username,(const uint8_t*)"LABUSER     ",12)) failed|=1u;
    if (service.privileges!=0x123u || disk.opened || disk.connected) failed|=2u;
    if (port_uaf_validate(&service,labuser,7,(const uint8_t*)"bad",3,
                           &record,sizeof record)!=PORT_UAF_INVPWD) failed|=4u;
    if (port_uaf_searchuser(&service,(const uint8_t*)"missing",7,
                           &record,sizeof record)!=PORT_UAF_INVUSR ||
        !bytes_equal(record.username,(const uint8_t*)"DEFAULT     ",12)) failed|=8u;
    if (port_uaf_validate(&service,(const uint8_t*)"DEFAULT",7,password,8,
                          &record,sizeof record)!=PORT_UAF_INVUSR) failed|=16u;
    if (port_uaf_searchuser(&service,(const uint8_t*)"purdyusr",8,
                           &record,sizeof record)!=PORT_UAF_NORMAL ||
        record.algorithm!=1) failed|=32u;
    if (port_uaf_validate(&service,(const uint8_t*)"purdyusr",8,password,8,
                          &record,sizeof record)!=PORT_UAF_NORMAL) failed|=64u;
    if (port_uaf_validate(&service,labuser,7,password,8,
                          &record,64)!=PORT_UAF_BAD_ARG) failed|=128u;
    if (port_uaf_validate(&service,(const uint8_t*)"system",6,
                          (const uint8_t*)"manager",7,
                          &record,sizeof record)!=PORT_UAF_NORMAL ||
        !bytes_equal(record.username,(const uint8_t*)"SYSTEM      ",12) ||
        record.hash[0]!=0x5cu || record.hash[1]!=0x24u ||
        record.hash[2]!=0x9eu || record.hash[3]!=0x3au)
        failed|=256u;
    if (port_uaf_validate(&service,(const uint8_t*)"system",6,
                          (const uint8_t*)"BAD",3,
                          &record,sizeof record)!=PORT_UAF_INVPWD)
        failed|=512u;
    return failed;
}
