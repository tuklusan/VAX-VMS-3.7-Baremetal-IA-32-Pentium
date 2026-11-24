#include "vms_iopost_end.h"
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

struct mapping { uint32_t va; uint8_t bytes[96]; };
struct rig { struct mapping maps[2]; uint32_t calls,pte[8],pages[8]; };
static void wr16(uint8_t *p,uint16_t v){p[0]=(uint8_t)v;p[1]=(uint8_t)(v>>8);}
static void wr32(uint8_t *p,uint32_t v){
    p[0]=(uint8_t)v;p[1]=(uint8_t)(v>>8);
    p[2]=(uint8_t)(v>>16);p[3]=(uint8_t)(v>>24);
}
static int resolve(void *ctx,uint32_t va,const uint8_t **out,size_t *bytes){
    struct rig *r=ctx;
    for(unsigned i=0;i<2u;i++)if(r->maps[i].va==va){
        *out=r->maps[i].bytes;*bytes=sizeof r->maps[i].bytes;return 0;
    }
    return -1;
}
static void unlock(void *ctx,uint32_t pte,uint32_t pages){
    struct rig *r=ctx;assert(r->calls<8u);
    r->pte[r->calls]=pte;r->pages[r->calls]=pages;++r->calls;
}
int main(void){
    _Alignas(4) uint8_t irp[PORT_V37_IRP_SIZE]={0};
    struct rig r={.maps={{.va=0x2000u},{.va=0x2010u}}};
    const struct port_v37_iopost_unlock_ops ops={&r,resolve,unlock,2u};
    wr16(irp+42u,UINT16_C(1u<<11));
    wr32(irp+44u,0x1000u);wr16(irp+48u,500u);wr32(irp+52u,24u);
    wr32(irp+84u,0x2000u);
    uint8_t *a=r.maps[0].bytes,*b=r.maps[1].bytes;
    wr16(a+42u,UINT16_C(1u<<11));
    wr32(a+44u,0x3000u);wr16(a+48u,0u);wr32(a+52u,1u);
    wr32(a+56u,0x4000u);wr16(a+60u,511u);wr32(a+64u,2u);
    wr32(a+84u,0x2010u);
    wr32(b+44u,0u);wr32(b+56u,0x5000u);wr16(b+60u,0u);wr32(b+64u,0u);
    assert(port_v37_iopost_unlock_direct(irp,sizeof irp,&ops)==PORT_V37_IOPOST_OK);
    assert(r.calls==3u&&r.pte[0]==0x1000u&&r.pages[0]==2u&&
           r.pte[1]==0x3000u&&r.pages[1]==1u&&
           r.pte[2]==0x4000u&&r.pages[2]==2u);
    r.calls=0u;wr16(b+42u,UINT16_C(1u<<11));
    assert(port_v37_iopost_unlock_direct(irp,sizeof irp,&ops)==PORT_V37_IOPOST_OK);
    assert(r.calls==3u);
    wr16(b+42u,0u);
    r.calls=0u;
    wr32(a+84u,0xdead0000u);
    assert(port_v37_iopost_unlock_direct(irp,sizeof irp,&ops)==PORT_V37_IOPOST_BAD);
    assert(r.calls==0u);
    r.calls=0u;wr16(irp+42u,UINT16_C(1u<<4));
    assert(port_v37_iopost_unlock_direct(irp,sizeof irp,&ops)==PORT_V37_IOPOST_UNLOCK_DEFER);
    assert(r.calls==0u);
    r.calls=0u;wr16(irp+42u,UINT16_C(1u<<2));
    assert(port_v37_iopost_unlock_direct(irp,sizeof irp,&ops)==PORT_V37_IOPOST_UNLOCK_DEFER);
    assert(r.calls==0u);
    r.calls=0u;wr16(irp+42u,0u);wr32(irp+44u,0x6000u);
    wr16(irp+48u,511u);wr32(irp+52u,2u);
    assert(port_v37_iopost_unlock_direct(irp,sizeof irp,&ops)==PORT_V37_IOPOST_OK);
    assert(r.calls==1u&&r.pte[0]==0x6000u&&r.pages[0]==2u);
    puts("IOCIOPOST direct-I/O locked-page release and IRPE chain PASS");
    return 0;
}
