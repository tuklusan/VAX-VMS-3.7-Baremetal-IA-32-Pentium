#include <assert.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include "vms_ne2k.h"
struct fake {
    uint8_t page,cr,reg0[32],reg1[32],ram[65536];
    unsigned prom_words,reset_writes,dma_address,dma_left,wire_bytes;
    int mode;
};
static const uint8_t mac[]={2,0x11,0x22,0x33,0x44,0x55};
static uint8_t in8(void *ctx,uint16_t addr) {
    struct fake *m=ctx;
    assert(addr>=0x300&&addr<=0x31f);
    unsigned reg=addr-0x300;
    if(reg==31) return m->mode==1?0xff:0x12;
    if(reg==0) return m->cr;
    if(reg==7&&m->page==0) return m->reg0[7];
    return m->page==1?m->reg1[reg]:m->reg0[reg];
}
static uint16_t in16(void *ctx,uint16_t addr) {
    struct fake *m=ctx;
    assert(addr==0x310);
    if(m->prom_words<16) {
        unsigned n=m->prom_words++;
        uint8_t v=n<6?mac[n]:0x57;
        if(m->mode==3&&n==0) v=0xff;
        return (uint16_t)(v|((uint16_t)v<<8));
    }
    assert(m->dma_left>=2);
    unsigned p=m->dma_address;
    uint16_t v=(uint16_t)(m->ram[p]|((uint16_t)m->ram[p+1]<<8));
    m->dma_address+=2;
    if(m->dma_address==(unsigned)m->reg0[2]*256)
        m->dma_address=(unsigned)m->reg0[1]*256;
    m->dma_left-=2;
    if(!m->dma_left&&m->mode!=4) m->reg0[7]|=0x40;
    return v;
}
static void out8(void *ctx,uint16_t addr,uint8_t v) {
    struct fake *m=ctx;
    assert(addr>=0x300&&addr<=0x31f);
    unsigned reg=addr-0x300;
    if(reg==31) {m->reset_writes++;if(m->mode!=2)m->reg0[7]=0x80;return;}
    if(reg==0) {
        m->page=(uint8_t)((v>>6)&3);m->cr=v;
        if(v==0x0a||v==0x12) {
            m->dma_address=(unsigned)m->reg0[8]|((unsigned)m->reg0[9]<<8);
            m->dma_left=(unsigned)m->reg0[10]|((unsigned)m->reg0[11]<<8);
        }
        if(v==0x26) {
            m->wire_bytes=(unsigned)m->reg0[5]|((unsigned)m->reg0[6]<<8);
            m->reg0[7] |= m->mode==5?0x08:0x02;
            m->cr=0x22;
        }
        return;
    }
    if(m->page==1) m->reg1[reg]=v;
    else if(reg==7) m->reg0[7]&=(uint8_t)~v;
    else m->reg0[reg]=v;
}
static void out16(void *ctx,uint16_t addr,uint16_t v) {
    struct fake *m=ctx;
    assert(addr==0x310&&m->dma_left>=2);
    m->ram[m->dma_address]=(uint8_t)v;
    m->ram[m->dma_address+1]=(uint8_t)(v>>8);
    m->dma_address+=2;
    m->dma_left-=2;
    if(!m->dma_left&&m->mode!=4)m->reg0[7]|=0x40;
}
static struct port_ne2k_bus setup(struct fake *m) {
    struct port_ne2k_bus b={in8,in16,out8,m,out16};
    return b;
}
static void inject(struct fake *m,uint8_t page,uint8_t next,const uint8_t *f,unsigned len) {
    unsigned a=(unsigned)page<<8;
    m->ram[a]=1;m->ram[a+1]=next;
    m->ram[a+2]=(uint8_t)(len+4);m->ram[a+3]=(uint8_t)((len+4)>>8);
    for(unsigned i=0;i<len;i++) {
        unsigned p=a+4+i;
        if(p>=0x8000)p=0x4600+(p-0x8000);
        m->ram[p]=f[i];
    }
    m->reg1[7]=next;
}
int main(void) {
    struct fake m={0};struct port_ne2k nic={0};
    const struct port_ne2k_bus b=setup(&m);
    assert(port_ne2k_probe(&nic,&b,0x300)==0);
    assert(nic.mac[0]==2&&nic.mac[5]==0x55);
    assert(nic.rx_start==0x46&&nic.rx_stop==0x80);
    assert(m.prom_words==16&&m.reset_writes==1);
    assert(m.reg0[12]==0x0c&&m.reg0[15]==0);
    assert(m.reg1[7]==0x47&&m.reg1[1]==2);
    assert(port_ne2k_pending(&nic,&b)==0);
    uint8_t f[1518]={0},got[1518]={0};
    for(unsigned i=0;i<sizeof f;i++)f[i]=(uint8_t)(i*7u+1u);
    assert(port_ne2k_send(&nic,&b,f,47)==0);
    assert(m.wire_bytes==60&&nic.tx_packets==1);
    assert(!memcmp(m.ram+0x4000,f,47));
    for(unsigned i=47;i<60;i++)assert(m.ram[0x4000+i]==0);
    assert(port_ne2k_send(&nic,&b,f,1518)==0);
    assert(m.wire_bytes==1518&&memcmp(m.ram+0x4000,f,1518)==0);
    assert(port_ne2k_send(&nic,&b,f,1519)==PORT_NE2K_INVALID);
    inject(&m,0x47,0x48,f,73);
    assert(port_ne2k_pending(&nic,&b)==1);
    assert(port_ne2k_recv(&nic,&b,got,sizeof got)==73);
    assert(!memcmp(got,f,73)&&m.reg0[3]==0x47);
    assert(port_ne2k_pending(&nic,&b)==0);
    /* Ring crossing: header at 0x7f00, data wraps at 0x8000. */
    m.reg0[3]=0x7e;m.reg1[7]=0x47;
    inject(&m,0x7f,0x47,f,373);
    memset(got,0,sizeof got);
    assert(port_ne2k_recv(&nic,&b,got,sizeof got)==373);
    assert(!memcmp(got,f,373)&&m.reg0[3]==0x46);
    assert(nic.rx_packets==2);
    inject(&m,0x47,0x48,f,64);
    assert(port_ne2k_recv(&nic,&b,got,20)==PORT_NE2K_BUFFER);
    assert(nic.rx_drops==1&&m.reg0[3]==0x47);
    /* Corrupt NEXT pointer, valid CURR: drain to current. */
    m.reg0[3]=0x47;
    inject(&m,0x48,0x49,f,60);
    m.ram[0x4801]=0xfe;
    assert(port_ne2k_recv(&nic,&b,got,sizeof got)==PORT_NE2K_CORRUPT);
    assert(nic.rx_drops==2&&m.reg0[3]==0x48);
    struct fake absent={.mode=1};const struct port_ne2k_bus ba=setup(&absent);
    assert(port_ne2k_probe(&nic,&ba,0x300)==PORT_NE2K_MISSING);
    struct fake timeout={.mode=2};const struct port_ne2k_bus bt=setup(&timeout);
    assert(port_ne2k_probe(&nic,&bt,0x300)==PORT_NE2K_TIMEOUT);
    struct fake badmac={.mode=3};const struct port_ne2k_bus bm=setup(&badmac);
    assert(port_ne2k_probe(&nic,&bm,0x300)==PORT_NE2K_BAD_MAC);
    puts("M12 NE2000 host PASS: TX pad/full, RX, wrap, drops, reset faults");
    return 0;
}
