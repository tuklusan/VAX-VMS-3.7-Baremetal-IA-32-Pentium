#include "vms_ramcheck.h"
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
static uint8_t mem[3u*1024u*1024u+8192u];
struct model {uint32_t mirror_bytes;unsigned reads,writes;};
static uint32_t locate(struct model *m,uint32_t address) {
    if(m->mirror_bytes)address%=m->mirror_bytes;
    assert(address<sizeof(mem));return address;
}
static uint8_t get(void *ctx,uint32_t address) {
    struct model *m=ctx;++m->reads;return mem[locate(m,address)];
}
static void put(void *ctx,uint32_t address,uint8_t value) {
    struct model *m=ctx;++m->writes;mem[locate(m,address)]=value;
}
static void run(uint32_t mirror,int expected) {
    for(unsigned i=0;i<sizeof(mem);i++)mem[i]=(uint8_t)(i*47u+i/4096u);
    uint8_t baseline[4];
    struct model m={.mirror_bytes=mirror};
    const uint32_t probes[4]={0x500u,0x100500u,0x200500u,0x2ff500u};
    for(unsigned i=0;i<4;i++)baseline[i]=mem[locate(&m,probes[i])];
    const struct port_ram_probe_bus bus={get,put,&m};
    assert(port_ram_probe_3mib(&bus)==expected);
    assert(m.reads==8&&m.writes==8);
    for(unsigned i=0;i<4;i++)assert(mem[locate(&m,probes[i])]==baseline[i]);
}
int main(void) {
    assert(!port_ram_probe_3mib(0));
    run(0,1);
    run(1u*1024u*1024u,0); /* A20 disabled or 1 MiB mirror */
    run(2u*1024u*1024u,0); /* missing 2..3 MiB region */
    run(3u*1024u*1024u,1); /* 3 MiB covers the entire pool */
    puts("M44 non-destructive 1..3MiB RAM alias probe and restoration PASS");
}
