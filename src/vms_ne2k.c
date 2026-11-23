#include "vms_ne2k.h"
/* DP8390 register offsets; page 1 aliases page-0 registers. */
enum { REG_CR=0,REG_PSTART=1,REG_PSTOP=2,REG_BNRY=3,REG_TPSR=4,
       REG_TBCR0=5,REG_TBCR1=6,REG_ISR=7,REG_CURR=7,
       REG_RSAR0=8,REG_RSAR1=9,REG_RBCR0=10,REG_RBCR1=11,
       REG_RCR=12,REG_TCR=13,REG_DCR=14,REG_IMR=15,
       REG_DMA=16,REG_RESET=31,CR_STOP=0x21,CR_START=0x22,
       CR_PAGE1=0x62,CR_RREAD=0x0a,CR_RWRITE=0x12,CR_TX=0x26,
       ISR_RESET=0x80,ISR_RDC=0x40,ISR_TX_OK=0x02,ISR_TX_ERR=0x08 };
static uint8_t rd(const struct port_ne2k_bus *b,uint16_t p,uint8_t r) {
    return b->in8(b->ctx,(uint16_t)(p+r));
}
static void wr(const struct port_ne2k_bus *b,uint16_t p,uint8_t r,uint8_t v) {
    b->out8(b->ctx,(uint16_t)(p+r),v);
}
static int wait_isr(const struct port_ne2k_bus *b,uint16_t base,uint8_t mask) {
    for (unsigned i=0;i<100000;i++)
        if (rd(b,base,REG_ISR)&mask) return PORT_NE2K_OK;
    return PORT_NE2K_TIMEOUT;
}
static void dma_begin(const struct port_ne2k_bus *b,uint16_t base,
                      uint16_t addr,uint16_t count,uint8_t cmd) {
    wr(b,base,REG_CR,CR_START);
    wr(b,base,REG_ISR,ISR_RDC);
    wr(b,base,REG_RSAR0,(uint8_t)addr);
    wr(b,base,REG_RSAR1,(uint8_t)(addr>>8));
    wr(b,base,REG_RBCR0,(uint8_t)count);
    wr(b,base,REG_RBCR1,(uint8_t)(count>>8));
    wr(b,base,REG_CR,cmd);
}
static int dma_read(const struct port_ne2k *nic,const struct port_ne2k_bus *b,
                    uint16_t addr,uint8_t *buf,unsigned n) {
    unsigned physical=(n+1u)&~1u;
    dma_begin(b,nic->base,addr,(uint16_t)physical,CR_RREAD);
    for (unsigned i=0;i<physical;i+=2) {
        uint16_t v=b->in16(b->ctx,(uint16_t)(nic->base+REG_DMA));
        if(i<n) buf[i]=(uint8_t)v;
        if(i+1<n) buf[i+1]=(uint8_t)(v>>8);
    }
    int rc=wait_isr(b,nic->base,ISR_RDC);
    wr(b,nic->base,REG_ISR,ISR_RDC);
    wr(b,nic->base,REG_CR,CR_START);
    return rc;
}
static int dma_write(const struct port_ne2k *nic,const struct port_ne2k_bus *b,
                     uint16_t addr,const uint8_t *buf,unsigned n,unsigned padded) {
    dma_begin(b,nic->base,addr,(uint16_t)padded,CR_RWRITE);
    for (unsigned i=0;i<padded;i+=2) {
        uint8_t lo=i<n?buf[i]:0, hi=(i+1<n)?buf[i+1]:0;
        b->out16(b->ctx,(uint16_t)(nic->base+REG_DMA),
                 (uint16_t)(lo|((uint16_t)hi<<8)));
    }
    int rc=wait_isr(b,nic->base,ISR_RDC);
    wr(b,nic->base,REG_ISR,ISR_RDC);
    wr(b,nic->base,REG_CR,CR_START);
    return rc;
}
int port_ne2k_probe(struct port_ne2k *nic,const struct port_ne2k_bus *bus,uint16_t base) {
    if (!nic || !bus || !bus->in8 || !bus->in16 || !bus->out8 ||
        base<0x100 || base>0xffe0u) return PORT_NE2K_INVALID;
    uint8_t reset=rd(bus,base,REG_RESET);
    if (reset==0xff) return PORT_NE2K_MISSING;
    wr(bus,base,REG_RESET,reset);
    if (wait_isr(bus,base,ISR_RESET)) return PORT_NE2K_TIMEOUT;
    wr(bus,base,REG_CR,CR_STOP);
    wr(bus,base,REG_DCR,0x49); /* WTS=1; 8 byte FIFO threshold */
    wr(bus,base,REG_RBCR0,0);wr(bus,base,REG_RBCR1,0);
    wr(bus,base,REG_RCR,0x20);wr(bus,base,REG_TCR,0x02);
    wr(bus,base,REG_PSTART,0x46);wr(bus,base,REG_PSTOP,0x80);
    wr(bus,base,REG_BNRY,0x46);wr(bus,base,REG_TPSR,0x40);
    wr(bus,base,REG_ISR,0xff);wr(bus,base,REG_IMR,0);
    wr(bus,base,REG_RSAR0,0);wr(bus,base,REG_RSAR1,0);
    wr(bus,base,REG_RBCR0,32);wr(bus,base,REG_RBCR1,0);
    wr(bus,base,REG_CR,CR_RREAD);
    uint8_t prom[16];
    for (unsigned i=0;i<16;i++) prom[i]=(uint8_t)bus->in16(bus->ctx,(uint16_t)(base+REG_DMA));
    int all_zero=1,all_ff=1;
    for (unsigned i=0;i<6;i++) {
        if (prom[i]!=0) all_zero=0;
        if (prom[i]!=0xff) all_ff=0;
    }
    if ((prom[0]&1u)||all_zero||all_ff) return PORT_NE2K_BAD_MAC;
    wr(bus,base,REG_ISR,ISR_RDC);
    wr(bus,base,REG_CR,CR_PAGE1);
    for (unsigned i=0;i<6;i++) {
        nic->mac[i]=prom[i];wr(bus,base,(uint8_t)(1+i),prom[i]);
    }
    wr(bus,base,REG_CURR,0x47);
    for (unsigned i=0;i<8;i++) wr(bus,base,(uint8_t)(8+i),0xff);
    wr(bus,base,REG_CR,CR_START);
    wr(bus,base,REG_TCR,0);wr(bus,base,REG_RCR,0x0c);
    nic->base=base;nic->tx_start=0x40;nic->rx_start=0x46;nic->rx_stop=0x80;
    ++nic->reset_count;
    nic->rx_packets=nic->tx_packets=nic->rx_drops=nic->tx_errors=0;
    return PORT_NE2K_OK;
}
static int ring_indices(const struct port_ne2k *nic,const struct port_ne2k_bus *b,
                        uint8_t *current,uint8_t *next) {
    wr(b,nic->base,REG_CR,CR_PAGE1);
    *current=rd(b,nic->base,REG_CURR);
    wr(b,nic->base,REG_CR,CR_START);
    uint8_t boundary=rd(b,nic->base,REG_BNRY);
    *next=(uint8_t)(boundary+1);
    if (*next>=nic->rx_stop) *next=nic->rx_start;
    if (*current<nic->rx_start||*current>=nic->rx_stop||
        boundary<nic->rx_start||boundary>=nic->rx_stop)
        return PORT_NE2K_CORRUPT;
    return PORT_NE2K_OK;
}
int port_ne2k_pending(struct port_ne2k *nic,const struct port_ne2k_bus *b) {
    if(!nic||!nic->base||!b||!b->in8||!b->out8) return PORT_NE2K_INVALID;
    uint8_t current,next;
    int rc=ring_indices(nic,b,&current,&next);
    return rc ? rc : (current!=next);
}
int port_ne2k_send(struct port_ne2k *nic,const struct port_ne2k_bus *b,
                   const uint8_t *frame,unsigned len) {
    if (!nic||!nic->base||!b||!b->in8||!b->out8||!b->out16||!frame||
        len<14||len>PORT_NE2K_FRAME_MAX) return PORT_NE2K_INVALID;
    uint16_t p=nic->base;
    wr(b,p,REG_CR,CR_START);
    if (rd(b,p,REG_CR)&0x04u) return PORT_NE2K_BUSY;
    unsigned wire=len<PORT_NE2K_FRAME_MIN?PORT_NE2K_FRAME_MIN:len;
    /* NE2000 word-DMA requires an even transfer length. */
    int rc=dma_write(nic,b,(uint16_t)(nic->tx_start<<8),frame,len,(wire+1u)&~1u);
    if(rc){++nic->tx_errors;return rc;}
    wr(b,p,REG_TPSR,nic->tx_start);
    wr(b,p,REG_TBCR0,(uint8_t)wire);
    wr(b,p,REG_TBCR1,(uint8_t)(wire>>8));
    wr(b,p,REG_ISR,(uint8_t)(ISR_TX_OK|ISR_TX_ERR));
    wr(b,p,REG_CR,CR_TX);
    rc=wait_isr(b,p,(uint8_t)(ISR_TX_OK|ISR_TX_ERR));
    if(rc|| (rd(b,p,REG_ISR)&ISR_TX_ERR)) {
        ++nic->tx_errors;return rc?rc:PORT_NE2K_CORRUPT;
    }
    wr(b,p,REG_ISR,(uint8_t)(ISR_TX_OK|ISR_TX_ERR));
    ++nic->tx_packets;
    return PORT_NE2K_OK;
}
int port_ne2k_recv(struct port_ne2k *nic,const struct port_ne2k_bus *b,
                   uint8_t *frame,unsigned capacity) {
    if(!nic||!nic->base||!b||!b->in8||!b->in16||!b->out8||!frame)
        return PORT_NE2K_INVALID;
    uint8_t current,next;
    int rc=ring_indices(nic,b,&current,&next);
    if(rc) return rc;
    if(next==current) return 0;
    uint8_t hdr[4];
    rc=dma_read(nic,b,(uint16_t)(next<<8),hdr,4);
    if(rc) return rc;
    uint8_t nextpage=hdr[1];
    unsigned count=(unsigned)hdr[2]|((unsigned)hdr[3]<<8);
    int good=(hdr[0]&0x01u) && nextpage>=nic->rx_start &&
             nextpage<nic->rx_stop && nextpage!=next &&
             count>=64 && count<=PORT_NE2K_FRAME_MAX+4u;
    /* Bad next pointer cannot be trusted: drain the ring to CURR. */
    if(!good && (nextpage<nic->rx_start||nextpage>=nic->rx_stop||nextpage==next))
        nextpage=current;
    /* Advance BNRY, even when caller's buffer is too small. */
    if(good && count-4u>capacity) rc=PORT_NE2K_BUFFER;
    else if(!good) rc=PORT_NE2K_CORRUPT;
    else {
        unsigned n=count-4u;
        /* First frame byte follows the 4-byte NIC header. Remote DMA wraps
           at PSTOP; split at ring end so no transfer crosses the boundary. */
        unsigned addr=((unsigned)next<<8)+4;
        unsigned first=((unsigned)nic->rx_stop<<8)-addr;
        if(first>n) first=n;
        rc=dma_read(nic,b,(uint16_t)addr,frame,first);
        if(!rc && n>first)
            rc=dma_read(nic,b,(uint16_t)(nic->rx_start<<8),frame+first,n-first);
        if(!rc) {++nic->rx_packets;rc=(int)n;}
    }
    uint8_t boundary=(uint8_t)(nextpage-1u);
    if(boundary<nic->rx_start) boundary=(uint8_t)(nic->rx_stop-1u);
    wr(b,nic->base,REG_BNRY,boundary);
    if(rc<0) ++nic->rx_drops;
    return rc;
}

int port_ne2k_set_station(struct port_ne2k *nic,const struct port_ne2k_bus *b,
                          const uint8_t mac[6]) {
    if (!nic||!nic->base||!b||!b->out8||!mac||((mac[0]&1u)!=0))
        return PORT_NE2K_INVALID;
    wr(b,nic->base,REG_CR,CR_PAGE1);
    for(unsigned i=0;i<6;i++) {
        wr(b,nic->base,(uint8_t)(1+i),mac[i]);
        nic->mac[i]=mac[i];
    }
    wr(b,nic->base,REG_CR,CR_START);
    return PORT_NE2K_OK;
}
