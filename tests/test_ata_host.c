#include <assert.h>
#include <stdio.h>
#include <stdint.h>
#include "vms_ata.h"
struct mock { unsigned command,words,delay,drive,alt; uint8_t regs[8]; int broken; int selected; };
static uint8_t in8(void *p,uint16_t addr) {
    struct mock *m=p;
    if (addr==0x3f6) {++m->alt; return 0x50;}
    assert(addr==0x1f7);
    if (m->broken==1) return 0xff;
    if (m->broken==2) return 0x80;
    if (m->broken==3) return 1;
    /* Simulated BIOS left nonexistent primary slave selected. */
    if (!m->selected) return 0xff;
    return m->command ? 0x58 : 0x50;
}
static uint16_t in16(void *p,uint16_t addr) {
    struct mock *m=p;
    assert(addr==0x1f0 && m->command==0x20 && m->words<256);
    return (uint16_t)(m->words++ ^ 0xabcd);
}
static void out8(void *p,uint16_t addr,uint8_t value) {
    struct mock *m=p;
    if (addr==0x1f7) m->command=value;
    else {assert(addr>=0x1f2 && addr<=0x1f6);m->regs[addr-0x1f0]=value;
          if (addr==0x1f6) m->selected=1;}
}
int main(void) {
    struct mock m={0};
    struct port_ata_bus bus={in8,in16,out8,&m};
    uint16_t buf[256]={0};
    assert(port_ata_read28(&bus,0x01020304,buf)==0);
    assert(m.command==0x20 && m.words==256 && m.alt==4 && m.selected);
    assert(m.regs[6]==0xe1 && m.regs[2]==1);
    assert(m.regs[3]==4 && m.regs[4]==3 && m.regs[5]==2);
    for (unsigned i=0;i<256;i++) assert(buf[i]==(uint16_t)(i^0xabcd));
    assert(port_ata_read28(&bus,0x10000000,buf)==PORT_ATA_BAD_ARG);
    assert(port_ata_read28(&bus,1,0)==PORT_ATA_BAD_ARG);
    m.command=0;m.broken=1;
    assert(port_ata_read28(&bus,1,buf)==PORT_ATA_NO_DISK);
    m.broken=2;
    assert(port_ata_read28(&bus,1,buf)==PORT_ATA_TIMEOUT);
    m.broken=3;
    assert(port_ata_read28(&bus,1,buf)==PORT_ATA_DEVICE_ERROR);
    puts("M10 ATA host PASS: LBA28 port sequence, 256 words, error/timeout");
    return 0;
}
