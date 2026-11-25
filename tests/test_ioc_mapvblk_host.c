#include "vms_ioc_mapvblk.h"
#include "vms_v37_dyn_tags.h"
#include "vms_ini_syswcb.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>

struct fixture {
    uint8_t first[96],second[96];
    unsigned enters,leaves,hits,rvn_calls;
    uint32_t file_size,relative_result;
    int sequential;
};
static void put16(uint8_t *p,uint16_t v){p[0]=(uint8_t)v;p[1]=(uint8_t)(v>>8);}
static void put32(uint8_t *p,uint32_t v){put16(p,(uint16_t)v);put16(p+2,(uint16_t)(v>>16));}
static void extent(uint8_t *w,unsigned i,uint16_t count,uint32_t lbn){
    uint8_t *p=w+PORT_IOC_MAPVBLK_WCB_BYTES+6u*i;put16(p,count);put32(p+2,lbn);
}
static void setup(uint8_t *w,uint16_t nmap,uint32_t stvbn,uint8_t access){
    memset(w,0,96u);put16(w+8,(uint16_t)(48u+6u*nmap));
    w[10]=PORT_V37_DYN_WCB;w[11]=access;put32(w+16,0xaaaa0001u);
    put16(w+22,nmap);put32(w+24,0x3000u);put32(w+28,0x4000u);
    put32(w+44,stvbn);
}
static const uint8_t *resolve(void *ctx,uint32_t va,size_t minimum,size_t *cap){
    struct fixture *f=ctx;assert(minimum==48u);
    if(va==0x1000u){*cap=sizeof f->first;return f->first;}
    if(va==0x2000u){*cap=sizeof f->second;return f->second;}
    return 0;
}
static uint32_t enter(void *ctx,uint32_t ucb,uint32_t *cookie){
    struct fixture *f=ctx;assert(ucb==0x11110000u);f->enters++;*cookie=0x55u;return 1u;
}
static void leave(void *ctx,uint32_t ucb,uint32_t cookie){
    struct fixture *f=ctx;assert(ucb==0x11110000u&&cookie==0x55u);f->leaves++;
}
static uint32_t is_sequential(void *ctx,uint32_t ucb){
    struct fixture *f=ctx;assert(ucb==0x11110000u);return (uint32_t)f->sequential;
}
static uint32_t get_file_size(void *ctx,uint32_t fcb,uint32_t *blocks){
    struct fixture *f=ctx;assert(fcb==0x3000u);*blocks=f->file_size;return 1u;
}
static uint32_t relative_ucb(void *ctx,uint32_t rvt,uint8_t rvn,uint32_t *ucb){
    struct fixture *f=ctx;assert(rvt==0x4000u&&rvn==2u);f->rvn_calls++;
    *ucb=f->relative_result;return *ucb?1u:0u;
}
static void hit(void *ctx){((struct fixture *)ctx)->hits++;}
int main(void){
    struct fixture f={0};f.file_size=64u;f.relative_result=0x22220000u;
    const struct port_ioc_mapvblk_ops ops={&f,resolve,enter,leave,
        is_sequential,get_file_size,relative_ucb,hit};
    struct port_ioc_mapvblk_result r={0};
    setup(f.first,1u,1u,PORT_IOC_MAPVBLK_ACCESS_NOTFCP);extent(f.first,0,6u,100u);
    assert(port_ioc_mapvblk(2u,2048u,0x1000u,0x11110000u,&ops,&r)==PORT_IOC_MAPVBLK_OK);
    assert(r.lbn==101u&&r.unmapped_bytes==0u&&r.ucb==0x11110000u&&f.hits==1u);

    setup(f.first,2u,1u,PORT_IOC_MAPVBLK_ACCESS_NOTFCP);
    extent(f.first,0,2u,50u);extent(f.first,1,4u,52u);
    assert(port_ioc_mapvblk(2u,4096u,0x1000u,0x11110000u,&ops,&r)==PORT_IOC_MAPVBLK_OK);
    assert(r.lbn==51u&&r.unmapped_bytes==1536u&&f.hits==2u);
    extent(f.first,1,4u,60u);
    assert(port_ioc_mapvblk(2u,1024u,0x1000u,0x11110000u,&ops,&r)==PORT_IOC_MAPVBLK_OK);
    assert(r.lbn==51u&&r.unmapped_bytes==512u&&f.hits==3u);

    setup(f.first,1u,1u,0u);extent(f.first,0,10u,100u);f.file_size=3u;
    assert(port_ioc_mapvblk(2u,2048u,0x1000u,0x11110000u,&ops,&r)==PORT_IOC_MAPVBLK_OK);
    assert(r.lbn==101u&&r.unmapped_bytes==1024u);
    assert(port_ioc_mapvblk(4u,512u,0x1000u,0x11110000u,&ops,&r)==PORT_IOC_MAPVBLK_UNMAPPED);
    assert(r.unmapped_bytes==512u&&r.ucb==0xaaaa0001u);

    setup(f.first,1u,1u,PORT_IOC_MAPVBLK_ACCESS_NOTFCP);
    extent(f.first,0,4u,UINT32_C(0x02000064));
    assert(port_ioc_mapvblk(2u,512u,0x1000u,0x11110000u,&ops,&r)==PORT_IOC_MAPVBLK_OK);
    assert(r.lbn==101u&&r.ucb==0x22220000u&&f.rvn_calls==1u);

    setup(f.first,1u,1u,PORT_IOC_MAPVBLK_ACCESS_NOTFCP);
    put32(f.first+32u,0x2000u);extent(f.first,0,2u,10u);
    setup(f.second,1u,10u,PORT_IOC_MAPVBLK_ACCESS_NOTFCP);extent(f.second,0,5u,200u);
    assert(port_ioc_mapvblk(11u,512u,0x1000u,0x11110000u,&ops,&r)==PORT_IOC_MAPVBLK_OK);
    assert(r.lbn==201u);
    assert(port_ioc_mapvblk(5u,512u,0x1000u,0x11110000u,&ops,&r)==PORT_IOC_MAPVBLK_UNMAPPED);
    assert(r.ucb==0xaaaa0001u);

    setup(f.first,1u,1u,PORT_IOC_MAPVBLK_ACCESS_NOTFCP);put32(f.first+48u,0x33330000u);
    f.sequential=1;
    assert(port_ioc_mapvblk(100u,1600u,0x1000u,0x11110000u,&ops,&r)==PORT_IOC_MAPVBLK_OK);
    assert(r.ucb==0x33330000u&&r.unmapped_bytes==0u);f.sequential=0;

    f.first[10]=0u;
    unsigned before=f.enters;
    assert(port_ioc_mapvblk(1u,512u,0x1000u,0x11110000u,&ops,&r)==PORT_IOC_MAPVBLK_BADWCB);
    assert(f.enters==before+1u&&f.leaves==f.enters);
    assert(port_ioc_mapvblk(1u,512u,0x1000u,0x11110000u,0, &r)==PORT_IOC_MAPVBLK_BADPARAM);

    /* Consume the exact compact WCB record emitted by INIT's SYSWCB
     * transformer, rather than a separately hand-packed equivalent. */
    uint8_t boot_map[96]={0},template[48];
    put32(boot_map,24u);put32(boot_map+4u,7u);put32(boot_map+8u,128u);
    put32(boot_map+12u,10u);put32(boot_map+16u,140u);
    put32(boot_map+20u,3u);put32(boot_map+24u,160u);
    assert(port_ini_syswcb_template_init(template,sizeof template)==PORT_INI_SYSWCB_OK);
    const struct port_ini_syswcb_layout layout={48u,8u,22u};
    struct port_ini_syswcb generated={0};
    assert(port_ini_syswcb_from_boot_map(boot_map,sizeof boot_map,template,
        sizeof template,&layout,&generated)==PORT_INI_SYSWCB_OK);
    memcpy(f.first,generated.record,generated.record_length);
    assert(port_ioc_mapvblk(10u,512u,0x1000u,0x11110000u,&ops,&r)==PORT_IOC_MAPVBLK_OK);
    assert(r.lbn==142u&&r.unmapped_bytes==0u);
    puts("IOSUBRAMS IOC$MAPVBLK WCB/FM2/EOF/contiguity/RVN/SYSWCB checks PASS");
    return 0;
}
