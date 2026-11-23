#include "vms_f11_ioc.h"
/* M182: DQ FUNCXT -> IOCIOPOST, deliver just one private IOSB. */
uint32_t port_f11_ioc_post(struct port_f11_ioc_file *f){
 if(!f||!f->queue)return PORT_F11_IOC_BAD;
 if(!f->pending)return PORT_F11_IOC_BUSY;
 struct port_dq_ioc_iosb iosb={0};
 uint32_t st=port_dq_ioc_service(f->queue,&iosb);
 if(st!=PORT_DQ_IOC_OK&&st!=PORT_DQ_IOC_IO)return PORT_F11_IOC_BUSY;
 if(iosb.tag!=f->pending_tag)return PORT_F11_IOC_BAD;
 f->pending=0;f->pending_tag=0;
 f->reads++;
 return st==PORT_DQ_IOC_OK&&iosb.bytes==512u?
  PORT_F11_IOC_OK:PORT_F11_IOC_IO;
}
