#include "vms_f11_chain.h"
/* Follow FH2$W_EXT_FID (offsets 14-19), as FIL$RDCHKFILHDR does.
 * Source normalized extent decoder is reused from SYSBOOT, not copied
 * into another set of diverging Files-11 pointer rules. */
uint32_t port_f11_chain_collect(const struct port_f11_boot_volume *volume,
    const struct port_dq_dir_fid *first,struct port_f11_chain *out){
    if(!volume||!volume->home||!volume->index||!volume->read_sector||
       !first||!first->number||!first->sequence||!out)
        return PORT_F11_CHAIN_BAD;
    struct port_f11_chain scratch={0};
    scratch.first=*first;
    struct port_dq_dir_fid next=*first;
    struct port_dq_dir_fid visited[PORT_F11_CHAIN_MAX_HEADERS];
    unsigned nvisited=0u;
    while(next.number){
        if(nvisited>=PORT_F11_CHAIN_MAX_HEADERS)return PORT_F11_CHAIN_CYCLE;
        for(unsigned i=0u;i<nvisited;i++){
            if(visited[i].number==next.number&&
               visited[i].sequence==next.sequence&&
               visited[i].volume==next.volume)return PORT_F11_CHAIN_CYCLE;
        }
        visited[nvisited++]=next;
        uint8_t fh2[512];uint32_t hdr_lbn=0u;
        const uint32_t hdr=port_f11_boot_header(volume,&next,&hdr_lbn,fh2);
        if(hdr==PORT_F11_PATH_IO)return PORT_F11_CHAIN_IO;
        if(hdr==PORT_F11_PATH_HEADER)return PORT_F11_CHAIN_HEADER;
        if(hdr!=PORT_F11_PATH_OK)return PORT_F11_CHAIN_MAP;
        if(scratch.count==PORT_F11_CHAIN_MAX_EXTENTS)
            return PORT_F11_CHAIN_MAP;
        uint32_t found=0u;
        if(port_f11_collect_extents(fh2,scratch.extents+scratch.count,
             PORT_F11_CHAIN_MAX_EXTENTS-scratch.count,
             &found)!=PORT_F11_HOME_OK||!found)
            return PORT_F11_CHAIN_MAP;
        for(unsigned i=0;i<found;i++){
            const struct port_f11_extent *e=&scratch.extents[scratch.count+i];
            if(!e->blocks||!e->lbn||
               (uint64_t)e->lbn+e->blocks>UINT32_C(0x10000000)||
               UINT32_MAX-scratch.blocks<e->blocks)
                return PORT_F11_CHAIN_MAP;
            if(!scratch.blocks)scratch.first_lbn=e->lbn;
            scratch.blocks+=e->blocks;
        }
        scratch.count+=found;
        next.number=port_dq_index_word(fh2,14u);
        next.sequence=port_dq_index_word(fh2,16u);
        next.volume=port_dq_index_word(fh2,18u);
        if(!next.number&&(next.sequence||next.volume))
            return PORT_F11_CHAIN_HEADER;
        if(next.number&&!next.sequence)return PORT_F11_CHAIN_HEADER;
    }
    scratch.headers=nvisited;
    *out=scratch;
    return PORT_F11_CHAIN_OK;
}
