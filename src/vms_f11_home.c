#include "vms_f11_home.h"
static uint16_t read_word(const uint8_t *p) {
    return (uint16_t)(p[0] | ((uint16_t)p[1]<<8));
}
static uint32_t read_long(const uint8_t *p) {
    return (uint32_t)p[0] | ((uint32_t)p[1]<<8) |
           ((uint32_t)p[2]<<16) | ((uint32_t)p[3]<<24);
}
int port_f11_checksum2(uint8_t block[512], unsigned byte_offset) {
    if(!block || byte_offset>510u || (byte_offset&1u)) return 0;
    uint16_t sum=0;
    for(unsigned i=0;i<byte_offset;i+=2u)
        sum=(uint16_t)(sum+read_word(block+i));
    int same=read_word(block+byte_offset)==sum;
    block[byte_offset]=(uint8_t)sum;
    block[byte_offset+1u]=(uint8_t)(sum>>8);
    return same;
}
uint32_t port_f11_check_home2(uint8_t block[512], uint32_t expected_lbn,
                               const char *label,size_t len,
                               uint32_t label_mismatch,int *ods2) {
    if(!block || !ods2 || (len && !label))return 0u;
    if(read_long(block)!=expected_lbn ||
       read_long(block+8u)==0u || read_word(block+14u)==0u ||
       read_word(block+16u)==0u || read_word(block+18u)==0u ||
       read_word(block+20u)==0u || read_word(block+22u)==0u ||
       read_long(block+24u)==0u || read_long(block+28u)==0u ||
       read_word(block+32u)==0u || read_word(block+34u)==0u)
        return 0u;
    /* CHKHM2 checks both checksums, in order, and stops if either is bad. */
    if(!port_f11_checksum2(block,58u) ||
       !port_f11_checksum2(block,510u))return 0u;
    *ods2=1;
    /* The volume label in HM2$T_VOLNAME is blank-padded to 12 bytes. */
    for(unsigned i=0;i<12u || i<len;i++) {
        unsigned char requested=i<len?(unsigned char)label[i]:' ';
        unsigned char actual=i<12u?block[472u+i]:' ';
        if(requested!=actual)return label_mismatch;
    }
    return 1u;
}

/* MOUNT/SRC/CHKHM1.B32 CHECK_HOMEBLK1.  ODS-1 has no home LBN field.
 * Check the source-defined nonzero fields, both sums, structure version,
 * and the zero-padded volume label.  ERR_EXIT uses the supplied VMS code. */
uint32_t port_f11_check_home1(uint8_t block[512],
                              const char *label,size_t len,
                              uint32_t label_mismatch,
                              uint32_t file_structure_error){
    if(!block || (len && !label))return 0u;
    if(!read_word(block+0u) || !read_long(block+2u) ||
       !read_word(block+6u) || !read_word(block+8u))return 0u;
    if(!port_f11_checksum2(block,58u) ||
       !port_f11_checksum2(block,510u))return 0u;
    const uint16_t level=read_word(block+12u);
    if(level!=257u && level!=258u)return file_structure_error;
    for(size_t i=0;i<12u || i<len;i++){
        unsigned char expected=i<len?(unsigned char)label[i]:0u;
        unsigned char actual=i<12u?block[14u+i]:0u;
        if(expected!=actual)return label_mismatch;
    }
    return 1u;
}
