#ifndef VMS_NATIVE_UAF_FIXTURE_H
#define VMS_NATIVE_UAF_FIXTURE_H
#include <stdint.h>
#include <string.h>
#include "vms_validate.h"
#include "vms_uaf_v37.h"
/* A separate, real byte-for-byte test fixture for the native indexed
 * UAF backend. The Files-11 system disk is not a raw V37UAFIX device:
 * using LBA 3 on portdisk.img was a false assumption inherited from M73. */
struct uaf_fixture {uint8_t sectors[8][512];};
static int read_sector(void *ctx,uint32_t lba,uint16_t out[256]){
    const struct uaf_fixture *disk=(const struct uaf_fixture *)ctx;
    if(!disk || !out || lba>=8u)return -1;
    for(unsigned i=0;i<256u;i++)
        out[i]=(uint16_t)(disk->sectors[lba][2u*i]|
                          ((uint16_t)disk->sectors[lba][2u*i+1u]<<8u));
    return 0;
}
static int prepare_uaf_fixture(struct uaf_fixture *disk){
    memset(disk,0,sizeof *disk);
    uint8_t *dir=disk->sectors[3];
    memcpy(dir,"V37UAFIX",8);dir[8]=2u;dir[9]=4u;
    static const struct {char name[13];char password[10];uint8_t algorithm;}
        accounts[]={ {"DEFAULT     ","PASSWORD",0},
                     {"LABUSER     ","PASSWORD",0},
                     {"PURDYUSR    ","PASSWORD",1},
                     {"SYSTEM      ","MANAGER",0} };
    for(unsigned i=0;i<4;i++){
        const uint8_t *key=(const uint8_t *)accounts[i].name;
        uint8_t *d=dir+16u+i*16u;
        memcpy(d,key,12u);
        d[12]=(uint8_t)(4u+i);
        uint8_t *rec=disk->sectors[4u+i];
        memcpy(rec+VMS37_UAF_USERNAME,key,12u);
        rec[VMS37_UAF_ENCRYPT]=accounts[i].algorithm;
        size_t len=strlen(accounts[i].password);
        if(!port_hpwd(rec+VMS37_UAF_HASH,
                      (const uint8_t *)accounts[i].password,(uint16_t)len,
                      accounts[i].algorithm,0u,key,12u))return 0;
    }
    return 1;
}
#define SET_LABUSER_UAF_FLAGS(disk,flags) \
    ((disk)->sectors[5][VMS37_UAF_FLAGS]=(uint8_t)(flags))

#endif
