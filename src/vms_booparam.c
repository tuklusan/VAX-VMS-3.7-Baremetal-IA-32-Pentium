#include "vms_booparam.h"

void port_boo_parameters_init(struct port_boo_parameters *p){
    if(!p)return;
    uint8_t *dst=(uint8_t *)(void *)p;
    for(unsigned i=0;i<sizeof *p;i++)dst[i]=0;
}
void port_boo_parameters_handoff(struct port_boo_parameters *to,
                                  const struct port_boo_parameters *from){
    if(!to||!from)return;
    uint8_t *dst=(uint8_t *)(void *)to;
    const uint8_t *src=(const uint8_t *)(const void *)from;
    for(unsigned i=0;i<sizeof *to;i++)dst[i]=src[i];
}
