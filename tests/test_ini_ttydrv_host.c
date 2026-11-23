#include "vms_ini_ttydrv.h"
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#define CHECK(x) do{if(!(x)){fprintf(stderr,"FAIL %s:%d %s\n",__FILE__,__LINE__,#x);return 1;}}while(0)
static void w16(uint8_t *p,uint16_t v){p[0]=(uint8_t)v;p[1]=(uint8_t)(v>>8);}
static void w32(uint8_t *p,uint32_t v){
    for(unsigned i=0;i<4;i++)p[i]=(uint8_t)(v>>(8u*i));
}
static uint32_t r32(const uint8_t *p){
    return (uint32_t)p[0]|(uint32_t)p[1]<<8|
           (uint32_t)p[2]<<16|(uint32_t)p[3]<<24;
}
int main(void){
    uint8_t ram[512] __attribute__((aligned(16)));
    memset(ram,0xa5,sizeof ram);
    const uint32_t guest=0x360000u;
    /* A source-format DPTAB at offset 16, 56 bytes long. Class vector
     * begins 96 bytes after DPT, with six relative entries + zero. */
    uint8_t *dpt=ram+16u,*vec=dpt+96u;
    memset(dpt,0,56u);memset(vec,0,28u);
    w16(dpt+8u,400u);dpt[10u]=0x37u;
    w16(dpt+14u,188u);w16(dpt+30u,96u);
    uint32_t original[6]={0x180u,0x1a0u,0x1c0u,0x1d0u,0x200u,0x220u};
    for(unsigned i=0;i<6;i++)w32(vec+4u*i,original[i]);
    struct port_ini_tty_class result={0};
    uint8_t saved[512];memcpy(saved,ram,sizeof ram);
    CHECK(port_ini_tty_class_relocate(ram,sizeof ram,guest,16u,0x37u,
                                      187u,&result)==PORT_INI_TTY_TOO_SMALL_UCB);
    CHECK(!memcmp(ram,saved,sizeof ram));
    CHECK(port_ini_tty_class_relocate(ram,sizeof ram,guest,16u,0x38u,
                                      188u,&result)==PORT_INI_TTY_BAD_DPT);
    CHECK(!memcmp(ram,saved,sizeof ram));
    CHECK(port_ini_tty_class_relocate(ram,sizeof ram,guest,16u,0x37u,
                                      188u,&result)==PORT_INI_TTY_OK);
    CHECK(result.dpt_address==guest+16u&&result.vector_address==guest+112u);
    CHECK(result.ddt_address==guest+16u+0x200u);
    CHECK(result.getnext_address==guest+16u+0x180u);
    CHECK(result.putnext_address==guest+16u+0x1a0u);
    CHECK(result.relocated_entries==6u&&result.required_console_ucb_bytes==188u);
    CHECK(r32(vec+24u)==0u);
    for(unsigned i=0;i<6;i++)CHECK(r32(vec+4u*i)==guest+16u+original[i]);
    CHECK(ram[15u]==0xa5u&&ram[511u]==0xa5u);
    memcpy(ram,saved,sizeof ram);
    w16(dpt+30u,492u);
    CHECK(port_ini_tty_class_relocate(ram,sizeof ram,guest,16u,0x37u,
                                      188u,&result)==PORT_INI_TTY_BAD_VECTOR);
    memcpy(ram,saved,sizeof ram);
    w32(vec+24u,0x250u);
    memset(vec+28u,0x11u,sizeof ram-16u-96u-28u);
    CHECK(port_ini_tty_class_relocate(ram,sizeof ram,guest,16u,0x37u,
                                      188u,&result)==PORT_INI_TTY_BAD_VECTOR);
    CHECK(r32(vec)==original[0]);
    memcpy(ram,saved,sizeof ram);
    w32(vec+4u,0u);
    CHECK(port_ini_tty_class_relocate(ram,sizeof ram,guest,16u,0x37u,
                                      188u,&result)==PORT_INI_TTY_BAD_VECTOR);
    CHECK(r32(vec)==original[0]);
    puts("INIT.MAR real DPTAB and TTY CLASS_VECTOR offset relocation PASS");
    return 0;
}
