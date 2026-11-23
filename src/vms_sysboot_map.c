#include "vms_sysboot_map.h"
uint32_t port_sysboot_mapvbn(const struct port_f11_extent *map,
                              uint32_t count,uint32_t vbn,
                              uint32_t *lbn,uint32_t *remaining,
                              uint32_t *next_index) {
    if(!map||!vbn||!lbn||!remaining||!next_index)
        return PORT_F11_HOME_BADARG;
    for(uint32_t index=0;index<count;index++){
        uint32_t size=map[index].blocks;
        if(!size || map[index].lbn+size<map[index].lbn)
            return PORT_F11_FILE_BAD_MAP;
        if(vbn<=size){
            uint32_t delta=vbn-1u;
            *lbn=map[index].lbn+delta;
            *remaining=size-delta;
            *next_index=index+1u;
            return PORT_F11_HOME_OK;
        }
        vbn-=size;
    }
    *lbn=*remaining=0u;
    *next_index=count;
    return PORT_F11_FILE_EOF;
}
uint32_t port_sysboot_setmap(const struct port_f11_extent *map,
                             uint32_t count,uint32_t first_vbn,
                             uint32_t requested,
                             struct port_f11_extent *result,
                             uint32_t capacity,uint32_t *used) {
    if(!map||!result||!capacity||!used||!first_vbn||!requested)
        return PORT_F11_HOME_BADARG;
    uint32_t lbn=0,remaining=0,next=0;
    uint32_t rc=port_sysboot_mapvbn(map,count,first_vbn,
                                      &lbn,&remaining,&next);
    if(rc!=PORT_F11_HOME_OK)return rc;
    *used=0u;
    while(requested){
        if(*used==capacity)return PORT_F11_FILE_BAD_MAP;
        uint32_t length=remaining<requested?remaining:requested;
        result[*used].blocks=length;
        result[*used].lbn=lbn;
        ++*used;requested-=length;
        if(!requested)break;
        if(next>=count)return PORT_F11_FILE_EOF;
        if(!map[next].blocks ||
           map[next].lbn+map[next].blocks<map[next].lbn)
            return PORT_F11_FILE_BAD_MAP;
        lbn=map[next].lbn;remaining=map[next].blocks;next++;
    }
    return PORT_F11_HOME_OK;
}

/* SYSBOOT's source-defined BOO$L_SYS_MAP byte stream is not an
 * instruction emulator. Every word is encoded as native little-endian
 * 32-bit data for INIT's later 8-to-6-byte retrieval-pointer conversion. */
static uint32_t normalized_get32(const uint8_t *p){
    return (uint32_t)p[0]|((uint32_t)p[1]<<8)|
           ((uint32_t)p[2]<<16)|((uint32_t)p[3]<<24);
}
static void normalized_put32(uint8_t *p,uint32_t value){
    p[0]=(uint8_t)value;p[1]=(uint8_t)(value>>8);
    p[2]=(uint8_t)(value>>16);p[3]=(uint8_t)(value>>24);
}
uint32_t port_sysboot_build_normalized_map(uint8_t *destination,
    size_t capacity,const struct port_f11_extent *extents,
    uint32_t extent_count,uint32_t minimum_blocks,
    struct port_sysboot_normalized_map *out){
    if(!destination||!extents||!out||!extent_count||!minimum_blocks||
       extent_count>UINT32_MAX/8u||
       (uintptr_t)destination&3u)return PORT_F11_HOME_BADARG;
    uint32_t extent_bytes=extent_count*8u;
    if(extent_bytes>UINT32_MAX-4u ||
       (uint64_t)extent_bytes+4u>capacity)
        return PORT_F11_FILE_BAD_MAP;
    uint64_t total=0u;
    for(uint32_t i=0u;i<extent_count;i++){
        uint32_t blocks=extents[i].blocks,lbn=extents[i].lbn;
        if(!blocks || blocks>UINT32_MAX-lbn ||
           total+blocks>UINT32_MAX)
            return PORT_F11_FILE_BAD_MAP;
        total+=blocks;
    }
    if(total<minimum_blocks)return PORT_F11_FILE_BAD_MAP;
    normalized_put32(destination,extent_bytes);
    for(uint32_t i=0;i<extent_count;i++){
        normalized_put32(destination+4u+8u*i,extents[i].blocks);
        normalized_put32(destination+8u+8u*i,extents[i].lbn);
    }
    *out=(struct port_sysboot_normalized_map){
        .record=destination,.extent_count=extent_count,
        .normalized_bytes=extent_bytes,.total_blocks=(uint32_t)total};
    return PORT_F11_HOME_OK;
}
uint32_t port_sysboot_check_normalized_map(const uint8_t *record,
    size_t capacity,uint32_t minimum_blocks,
    struct port_sysboot_normalized_map *out){
    if(!record||!out||capacity<12u||!minimum_blocks ||
       (uintptr_t)record&3u)return PORT_F11_HOME_BADARG;
    uint32_t bytes=normalized_get32(record);
    if(!bytes || (bytes&7u) || (uint64_t)bytes+4u>capacity)
        return PORT_F11_FILE_BAD_MAP;
    uint32_t count=bytes/8u;
    uint64_t total=0u;
    for(uint32_t i=0;i<count;i++){
        uint32_t blocks=normalized_get32(record+4u+8u*i);
        uint32_t lbn=normalized_get32(record+8u+8u*i);
        if(!blocks || blocks>UINT32_MAX-lbn ||
           total+blocks>UINT32_MAX)
            return PORT_F11_FILE_BAD_MAP;
        total+=blocks;
    }
    if(total<minimum_blocks)return PORT_F11_FILE_BAD_MAP;
    *out=(struct port_sysboot_normalized_map){
        .record=record,.extent_count=count,
        .normalized_bytes=bytes,.total_blocks=(uint32_t)total};
    return PORT_F11_HOME_OK;
}
