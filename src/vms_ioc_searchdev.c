#include "vms_ioc_phys.h"
/* IOSUBPAGD.MAR 302-303: physical search asks for the given unit. */
uint32_t port_v37_ioc_searchdev(const uint8_t *m,size_t n,uint32_t base,
     const struct port_v37_device_result *r,const char *text,size_t length,
     struct port_v37_ioc_result *out){
    return port_v37_ioc_search(m,n,base,r,text,length,PORT_V37_IOC_MODE_PHYSICAL,out);
}
