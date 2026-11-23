#include "vms_f11_ioc.h"
/* M179: original FIL$READVBN VBN cursor positioning, EOF inclusive. */
uint32_t port_f11_ioc_seek(struct port_f11_ioc_file *f,uint32_t at){
 if(!f||!f->chain||!f->queue)return PORT_F11_IOC_BAD;
 if(f->pending)return PORT_F11_IOC_BUSY;
 if(at>f->length)return PORT_F11_IOC_EOF;
 f->position=at;return PORT_F11_IOC_OK;
}
