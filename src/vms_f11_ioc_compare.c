#include "vms_f11_ioc.h"
/* M184: validate actual physical Files-11 media against resident IA-32 text. */
uint32_t port_f11_ioc_compare(struct port_f11_ioc_file *f,
 const uint8_t *expected,uint32_t count,uint32_t *equal){
 if(!f||!expected||!equal||!count||count>PORT_F11_IOC_MAX_READ)
  return PORT_F11_IOC_BAD;
 uint8_t bytes[PORT_F11_IOC_MAX_READ];uint32_t done=0;
 uint32_t st=port_f11_ioc_read(f,bytes,count,sizeof(bytes),&done);
 if(st!=PORT_F11_IOC_OK)return st;
 uint32_t n=0;
 while(n<count&&bytes[n]==expected[n])n++;
 *equal=n;
 return n==count?PORT_F11_IOC_OK:PORT_F11_IOC_DIFFERENT;
}
