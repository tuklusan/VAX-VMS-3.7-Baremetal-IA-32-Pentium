#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include "vms_iopost_end.h"
#define CHECK(x) do { if(!(x)){fprintf(stderr,"IRPE FAIL %d: %s\n",__LINE__,#x);return 1;} }while(0)
struct rig { uint8_t *first,*second; uint32_t freed[2],calls; };
static int resolve(void *v,uint32_t va,const uint8_t **p,size_t *n){
    struct rig *r=v;
    if(va==0x1000u)*p=r->first;
    else if(va==0x2000u)*p=r->second;
    else return -1;
    *n=PORT_V37_IRPE_BYTES;return 0;
}
static void release(void *v,uint32_t pid,uint32_t va){
    struct rig *r=v;
    if(pid!=0x10007u||r->calls>=2u){r->calls=99u;return;}
    r->freed[r->calls++]=va;
}
int test_iopost_irpe(void){
    _Alignas(4) uint8_t irp[PORT_V37_IRP_SIZE]={0};
    _Alignas(4) uint8_t first[PORT_V37_IRPE_BYTES]={0};
    _Alignas(4) uint8_t second[PORT_V37_IRPE_BYTES]={0};
    struct rig r={first,second,{0},0};
    struct port_v37_iopost_irpe_plan plan={0};
    port_v37_write16(irp+PORT_V37_IRP_STS,1u<<11);
    port_v37_write32(irp+PORT_V37_IRP_EXTEND,0x1000u);
    port_v37_write16(first+PORT_V37_IRPE_STS,1u<<11);
    port_v37_write32(first+PORT_V37_IRPE_EXTEND,0x2000u);
    CHECK(port_v37_iopost_irpe_plan(irp,sizeof irp,resolve,&r,2u,&plan)==PORT_V37_IOPOST_OK);
    CHECK(plan.count==2u&&plan.address[0]==0x1000u&&plan.address[1]==0x2000u);
    CHECK(port_v37_iopost_irpe_release(&plan,release,&r,0x10007u)==PORT_V37_IOPOST_OK);
    CHECK(r.calls==2u&&r.freed[0]==0x1000u&&r.freed[1]==0x2000u);
    memset(&plan,0xa5,sizeof plan);r.calls=0;
    CHECK(port_v37_iopost_irpe_plan(irp,sizeof irp,resolve,&r,1u,&plan)==PORT_V37_IOPOST_BAD);
    CHECK(port_v37_iopost_irpe_plan(irp,sizeof irp,0,0,0u,&plan)==PORT_V37_IOPOST_IRPE_DEFER);
    port_v37_write32(irp+PORT_V37_IRP_EXTEND,0u);
    CHECK(port_v37_iopost_irpe_plan(irp,sizeof irp,resolve,&r,2u,&plan)==PORT_V37_IOPOST_BAD);
    port_v37_write16(irp+PORT_V37_IRP_STS,0u);
    CHECK(port_v37_iopost_irpe_plan(irp,sizeof irp,0,0,0u,&plan)==PORT_V37_IOPOST_OK&&plan.count==0u);
    return 0;
}
