#include "vms_f11_ioc.h"
/* M183: FIL$READVBN partial-sector/extent read. Destination and position
 * commit only after entire bounded read succeeds; hardware failures report
 * bytes physically completed without exposing stale sector buffers. */
uint32_t port_f11_ioc_read(struct port_f11_ioc_file *f,uint8_t *out,
 uint32_t count,size_t capacity,uint32_t *done){
 if(!f||!out||!done||!f->chain||!f->queue||f->pending||
    !count||count>PORT_F11_IOC_MAX_READ||count>capacity)return PORT_F11_IOC_BAD;
 if(f->position>f->length||count>f->length-f->position)
  return PORT_F11_IOC_EOF;
 uint8_t temp[PORT_F11_IOC_MAX_READ];
 uint32_t at=f->position,got=0;
 while(got<count){
  uint32_t st=port_f11_ioc_stage(f);
  if(st!=PORT_F11_IOC_OK){f->position=at;return st;}
  st=port_f11_ioc_post(f);
  if(st!=PORT_F11_IOC_OK){f->position=at;*done=got;return st;}
  uint32_t n=512u-f->pending_within;
  if(n>count-got)n=count-got;
  for(uint32_t i=0;i<n;i++)temp[got+i]=f->sector[f->pending_within+i];
  f->position+=n;got+=n;
 }
 for(uint32_t i=0;i<count;i++)out[i]=temp[i];
 *done=count;return PORT_F11_IOC_OK;
}
