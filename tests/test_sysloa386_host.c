#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include "vms_sysloa386.h"
#define CHECK(e) do {if(!(e)){fprintf(stderr,"FAILED %s:%d %s\n",__FILE__,__LINE__,#e);return 1;}}while(0)
static struct port_sysloa386_gate gate(uint16_t low){
    return (struct port_sysloa386_gate){low,8u,0u,0x8eu,0x1u};
}
struct disk {uint8_t home[512];unsigned reads;int fail;};
static int read_sector(void *context,uint32_t lbn,uint16_t out[256]){
    struct disk *d=context;
    d->reads++;
    if(d->fail||lbn!=1u)return -1;
    memcpy(out,d->home,512u);
    return 0;
}
static uint16_t sum(const uint8_t *p,unsigned end){
    uint16_t n=0;
    for(unsigned i=0;i<end;i+=2u)n=(uint16_t)(n+p[i]+(p[i+1u]<<8));
    return n;
}
static void wr16(uint8_t *p,uint16_t v){p[0]=(uint8_t)v;p[1]=(uint8_t)(v>>8);}
static void wr32(uint8_t *p,uint32_t v){wr16(p,(uint16_t)v);wr16(p+2,(uint16_t)(v>>16));}
int main(void){
    struct port_sysloa386_cpu_result r={
        .cr0=0x80010001u,.cr3=0x11000u,.idt_base=0x13000u,
        .irq_ticks=19u,.idt_limit=2047u,.task_selector=0x48u,.kernel_cs=8u,
        .gp=gate(0x8000u),.pf=gate(0x9000u),.timer=gate(0xa000u)};
    CHECK(port_sysloa386_check_cpu(&r)==PORT_SYSLOA386_OK);
    r.timer.type=0x8fu;
    CHECK(port_sysloa386_check_cpu(&r)==PORT_SYSLOA386_BAD_CPU);
    r.timer.type=0x8eu;r.irq_ticks=0u;
    CHECK(port_sysloa386_check_cpu(&r)==PORT_SYSLOA386_BAD_CPU);
    r.irq_ticks=20u;r.cr0&=~0x10000u;
    CHECK(port_sysloa386_check_cpu(&r)==PORT_SYSLOA386_BAD_CPU);
    r.cr0|=0x10000u;r.task_selector=0x40u;
    CHECK(port_sysloa386_check_cpu(&r)==PORT_SYSLOA386_BAD_CPU);
    r.task_selector=0x48u;r.idt_limit=100u;
    CHECK(port_sysloa386_check_cpu(&r)==PORT_SYSLOA386_BAD_CPU);
    struct disk d={{0},0,0};
    wr32(d.home,1u);wr16(d.home+12u,0x0200u);
    wr16(d.home+14u,3u);wr32(d.home+24u,12u);
    wr32(d.home+28u,100u);wr16(d.home+34u,8u);
    wr16(d.home+58u,sum(d.home,58u));
    wr16(d.home+510u,sum(d.home,510u));
    struct port_sysloa386_boot_result b={0};
    struct port_sysloa386_boot_ops ops={&d,read_sector};
    CHECK(port_sysloa386_boot_probe(&ops,&b)==PORT_SYSLOA386_OK);
    CHECK(d.reads==1u&&b.home_block_lbn==1u&&b.cluster_factor==3u);
    d.home[19u]^=0x40u;
    CHECK(port_sysloa386_boot_probe(&ops,&b)==PORT_SYSLOA386_BAD_DISK);
    d.home[19u]^=0x40u;d.fail=1;
    CHECK(port_sysloa386_boot_probe(&ops,&b)==PORT_SYSLOA386_BAD_DISK);
    CHECK(port_sysloa386_boot_probe(0,&b)==PORT_SYSLOA386_BAD_ARGUMENT);
    puts("SYSLOA386 real CPU snapshot and Files-11 home validators PASS");
    return 0;
}
