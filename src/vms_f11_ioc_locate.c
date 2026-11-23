#include "vms_f11_ioc.h"
/* M180: one-origin VBN -> source GETRTRVPTR -> mapped physical LBN. */
uint32_t port_f11_ioc_locate(const struct port_f11_ioc_file *f,
 uint32_t *lbn,uint32_t *within){
 if(!f||!f->chain||!lbn||!within)return PORT_F11_IOC_BAD;
 if(f->position>=f->length)return PORT_F11_IOC_EOF;
 uint32_t l=0;
 if(port_f11_chain_lbn(f->chain,f->position/512u+1u,&l)
   !=PORT_F11_CHAIN_OK)return PORT_F11_IOC_MAP;
 *lbn=l;*within=f->position%512u;
 return PORT_F11_IOC_OK;
}
