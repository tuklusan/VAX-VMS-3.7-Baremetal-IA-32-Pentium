#include "vms_sysloavec.h"

/* Exact order from the original LOADVEC list. Type 1 contains data,
 * type 2 was VAX SCB-aligned code, and type 3 was packed code. */
static const uint8_t types[PORT_SYSLOA_COUNT]={
    2,2,2,2,2,2,2,
    3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,
    1
};
static uint16_t get16(const uint8_t *p) {
    return (uint16_t)((uint16_t)p[0]|((uint16_t)p[1]<<8));
}
static uint32_t get32(const uint8_t *p) {
    return (uint32_t)p[0]|((uint32_t)p[1]<<8)|
           ((uint32_t)p[2]<<16)|((uint32_t)p[3]<<24);
}
void port_sysloavec_init(struct port_sysloa_table *resident) {
    if(!resident)return;
    for(unsigned i=0;i<PORT_SYSLOA_COUNT;i++)resident->address[i]=0;
    resident->loaded=0;
}
void port_sysloavec_patch_init(struct port_sysloa_patch *patch) {
    if(!patch)return;
    patch->count=PORT_SYSLOA_PATCH_WORDS;
    for(unsigned i=0;i<PORT_SYSLOA_PATCH_WORDS;i++)patch->words[i]=0;
}
int port_sysloavec_link(struct port_sysloa_table *resident,
                        const void *loaded_image,uint32_t bytes,
                        uint8_t code_type) {
    if(!resident || !loaded_image || bytes<PORT_SYSLOA_HEADER_BYTES+5u*PORT_SYSLOA_COUNT+4u)
        return 0;
    const uint8_t *image=(const uint8_t *)loaded_image;
    /* The original PRMSW image begins with a twelve-byte DYN header.
     * The longword and shortword sizes both name its full byte length. */
    uint32_t extent=get32(image);
    if(extent>bytes||extent<PORT_SYSLOA_HEADER_BYTES+5u*PORT_SYSLOA_COUNT+4u||
       get32(image+4)!=0u||get16(image+8)!=(uint16_t)extent||
       image[10]!=code_type||image[11]!=0u)return 0;
    uint32_t cursor=PORT_SYSLOA_HEADER_BYTES;
    uintptr_t new_values[PORT_SYSLOA_COUNT];
    for(unsigned i=0;i<PORT_SYSLOA_COUNT;i++) {
        if(cursor>extent-5u||image[cursor]!=types[i])return 0;
        uint32_t offset_word=cursor+1u;
        int64_t relative=(int32_t)get32(image+offset_word);
        int64_t dest=(int64_t)offset_word+relative;
        if(dest<0||dest>=(int64_t)extent)return 0;
        new_values[i]=(uintptr_t)(image+(uint32_t)dest);
        cursor+=5u;
    }
    /* A negative longword is the loadable vector table's end marker. */
    if(cursor>extent-4u||get32(image+cursor)!=UINT32_MAX)return 0;
    for(unsigned i=0;i<PORT_SYSLOA_COUNT;i++)
        resident->address[i]=new_values[i];
    resident->loaded=1u;
    return 1;
}
uintptr_t port_sysloavec_address(const struct port_sysloa_table *resident,
                                 unsigned vector) {
    if(!resident||!resident->loaded||vector>=PORT_SYSLOA_COUNT)return 0;
    return resident->address[vector];
}
__attribute__((noreturn)) void port_sysloavec_unloaded(void) {
#if defined(__i386__) && !defined(PORT_SYSLOA_HOST_TEST)
    __asm__ volatile("cli\n1: hlt\njmp 1b");
#endif
    __builtin_trap();
    for(;;) { }
}
