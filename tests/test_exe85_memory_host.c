#include "vms_exe_memory85.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>
static uint32_t pde[1024],table[1024];
static uint8_t physical1[4096],physical2[4096];
static uint32_t failures;
static const uint32_t *get_table(void *u,uint32_t p){
    (void)u;return p==0x3000u?table:0;
}
static uint8_t *get_phys(void *u,uint32_t p){
    (void)u;return p==0x5000u?physical1:p==0x6000u?physical2:0;
}
static void fatal(void *u,uint32_t s){
    (void)u;assert(s==PORT_M85_LENVIO);failures++;
}
int main(void){
    pde[0]=0x3007;
    table[2]=0x5007;
    table[3]=0x6007;
    struct port_user_map user={pde,get_table,0};
    struct port_m85_pte_ops op={&user,{0x40000000u,0x40000000u,0x80000000u},0,fatal,0};
    struct port_m85_pte info={0};
    assert(port_mmg_pteindex(&op,0x2000,&info)==1);
    assert(info.vpn==2&&info.pde_index==0&&info.pte_index==2);
    assert(port_mmg_pteref(&op,0x2000,&info)==1&&info.pte==&table[2]);
    assert(info.value==0x5007);
    assert(port_mmg_svaptechk(&op,0x3000,&info)==1&&info.pte==&table[3]);
    assert(port_mmg_pteindex(&op,0x40000000u,&info)==PORT_M85_LENVIO);
    assert(port_mmg_pteindex(&op,0x80000000u,&info)==PORT_M85_LENVIO);
    assert(port_mmg_pteadrchk(&op,0x40000000u,&info)==PORT_M85_LENVIO);
    assert(failures==1);
    struct port_m85_buffer buf={0};
    buf.map=user;
    buf.physical=get_phys;
    assert(port_ioc_initbufwind(&buf,0x2fffu,1)==1);
    assert(port_ioc_putbyte(&buf,0x51)==1&&physical1[4095]==0x51);
    assert(port_ioc_putbyte(&buf,0x52)==1&&physical2[0]==0x52);
    assert(port_ioc_initbufwind(&buf,0x2fffu,0)==1);
    uint8_t byte=0;
    assert(port_ioc_getbyte(&buf,&byte)==1&&byte==0x51);
    assert(port_ioc_getbyte(&buf,&byte)==1&&byte==0x52);
    uint8_t in[8]={3,4,5,6,7,8,9,10};
    assert(port_ioc_initbufwind(&buf,0x2ffcu,1)==1);
    assert(port_ioc_movtouser(&buf,in,8)==1);
    assert(memcmp(physical1+4092,in,4)==0&&memcmp(physical2,in+4,4)==0);
    uint8_t out[8]={0};
    assert(port_ioc_initbufwind(&buf,0x2ffcu,0)==1);
    assert(port_ioc_movfruser(&buf,out,8)==1&&memcmp(out,in,8)==0);
    assert(port_ioc_initbufwind(&buf,0x3fffu,1)==1);
    uint8_t keep=physical2[4095];
    assert(port_ioc_movtouser(&buf,in,8)==PORT_M85_ACCVIO);
    assert(physical2[4095]==keep);
    table[3]&=~2u;
    assert(port_ioc_initbufwind(&buf,0x3000u,1)==PORT_M85_ACCVIO);
    assert(port_ioc_initbufwind(&buf,0x3000u,0)==1);
    puts("M85 resident PTE and user-buffer window tests PASS");
}
