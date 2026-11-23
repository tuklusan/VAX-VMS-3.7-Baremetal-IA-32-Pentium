#include "vms_dq_dir.h"
/* Level-2 variable-length directory entries. No zeroed or malformed
 * record is allowed to masquerade as a complete listing. */
static unsigned up(unsigned ch){return ch>='a'&&ch<='z'?ch-32u:ch;}
uint32_t port_dq_dir_find(const uint8_t b[512],const char *name,
                          size_t length,uint16_t version,
                          struct port_dq_dir_fid *fid){
    if(!b||!name||!length||length>80u||!fid||!version)
        return PORT_DQ_DIR_BAD;
    for(size_t i=0;i<length;i++)if(!name[i])return PORT_DQ_DIR_BAD;
    unsigned pos=0u;
    while(pos<512u){
        if(pos+2u>512u)return PORT_DQ_DIR_CORRUPT;
        const unsigned size=port_dq_index_word(b,pos);
        if(size==UINT16_MAX)return PORT_DQ_DIR_NOT_FOUND;
        if(size<12u||size>510u-pos||pos+size+2u>512u)
            return PORT_DQ_DIR_CORRUPT;
        const unsigned names=b[pos+5u];
        const unsigned end=pos+size+2u;
        const unsigned first=pos+6u+((names+1u)&~1u);
        if(!names||first+8u>end)return PORT_DQ_DIR_CORRUPT;
        unsigned match=names==length;
        if(match)for(size_t i=0;i<length;i++)
            if(up((unsigned char)b[pos+6u+i])!=up((unsigned char)name[i])){
                match=0u;break;
            }
        if(match){
            for(unsigned at=first;at+8u<=end;at+=8u){
                if(port_dq_index_word(b,at)!=version)continue;
                struct port_dq_dir_fid f={
                    port_dq_index_word(b,at+2u),
                    port_dq_index_word(b,at+4u),
                    port_dq_index_word(b,at+6u)};
                if(!f.number||!f.sequence)return PORT_DQ_DIR_CORRUPT;
                *fid=f;
                return PORT_DQ_DIR_OK;
            }
            return PORT_DQ_DIR_NOT_FOUND;
        }
        pos=end;
    }
    return PORT_DQ_DIR_NOT_FOUND;
}
