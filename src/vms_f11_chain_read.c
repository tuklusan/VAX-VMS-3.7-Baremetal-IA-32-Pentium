#include "vms_f11_chain.h"
/* FIL$READVBN with extension headers; only successful physical transfers
 * advance completed. A missing extent cannot become a synthesized EOF. */
uint32_t port_f11_chain_read(const struct port_f11_boot_volume *v,
    const struct port_f11_chain *chain,uint32_t off,uint32_t count,
    uint8_t *out,size_t capacity,uint32_t *completed){
    if(!v||!v->read_sector||!chain||!out||!completed||count>capacity||
       (uint64_t)off+count>UINT32_MAX)
        return PORT_F11_CHAIN_BAD;
    if(!count){*completed=0u;return PORT_F11_CHAIN_OK;}
    if(!chain->blocks||(uint64_t)off+count>(uint64_t)chain->blocks*512u)
        return PORT_F11_CHAIN_MAP;
    uint32_t n=0u;
    while(n<count){
        const uint32_t at=off+n;
        const uint32_t vbn=at/512u+1u;
        const uint32_t within=at%512u;
        uint32_t lbn=0u;
        if(port_f11_chain_lbn(chain,vbn,&lbn)!=PORT_F11_CHAIN_OK){
            *completed=n;return PORT_F11_CHAIN_MAP;
        }
        uint8_t sector[512];
        if(v->read_sector(v->context,lbn,sector)!=PORT_DQ_DIR_OK){
            *completed=n;return PORT_F11_CHAIN_IO;
        }
        const uint32_t bytes=count-n<512u-within?count-n:512u-within;
        for(uint32_t k=0u;k<bytes;k++)out[n+k]=sector[within+k];
        n+=bytes;
    }
    *completed=n;
    return PORT_F11_CHAIN_OK;
}
