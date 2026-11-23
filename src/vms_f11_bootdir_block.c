#include "vms_f11_bootdir.h"
/* FIL$FINDFILID level-2 directory entry: record length then 4-byte
 * metadata, 1-byte name length, even padding and 8-byte FID/version rows.
 * Word 0xFFFF terminates the VBN. Invalid records fail closed. */
uint32_t port_f11_bootdir_block(const uint8_t block[512],
    port_f11_bootdir_visit_fn visit,void *ctx,uint32_t *seen){
    if(!block||!visit||!seen)return PORT_F11_BOOTDIR_BAD;
    unsigned pos=0u;
    while(pos<512u){
        if(pos+2u>512u)return PORT_F11_BOOTDIR_CORRUPT;
        const unsigned n=port_dq_index_word(block,pos);
        if(n==UINT16_MAX)return PORT_F11_BOOTDIR_OK;
        if(n<12u||n>510u-pos||pos+n+2u>512u)
            return PORT_F11_BOOTDIR_CORRUPT;
        const unsigned end=pos+n+2u;
        const unsigned count=block[pos+5u];
        const unsigned first=pos+6u+((count+1u)&~1u);
        if(!count||count>80u||first+8u>end||(end-first)%8u)
            return PORT_F11_BOOTDIR_CORRUPT;
        for(unsigned at=first;at+8u<=end;at+=8u){
            const uint16_t version=port_dq_index_word(block,at);
            const struct port_dq_dir_fid f={port_dq_index_word(block,at+2u),
                port_dq_index_word(block,at+4u),port_dq_index_word(block,at+6u)};
            if(!version||!f.number||!f.sequence||*seen==UINT32_MAX)
                return PORT_F11_BOOTDIR_CORRUPT;
            ++*seen;
            if(!visit(ctx,block+pos+6u,count,version,&f))
                return PORT_F11_BOOTDIR_STOP;
        }
        pos=end;
    }
    return PORT_F11_BOOTDIR_OK;
}
