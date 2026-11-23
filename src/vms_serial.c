#include "vms_serial.h"
static int callbacks(const struct port_serial_io *io) {
    return io&&io->in&&io->out;
}
int port_serial_setup(struct port_serial *u,
                      const struct port_serial_io *io,uint16_t base) {
    if(!u||!callbacks(io)||base==0||base>0xfff8u)return -1;
    *u=(struct port_serial){0};u->base=base;
    if(io->in(io->context,(uint16_t)(base+5u))==0xffu)
        return 0; /* no UART at this address */
    io->out(io->context,(uint16_t)(base+1u),0); /* interrupts off */
    io->out(io->context,(uint16_t)(base+3u),0x80); /* divisor latch */
    io->out(io->context,(uint16_t)(base+0u),1); /* 115200 baud */
    io->out(io->context,(uint16_t)(base+1u),0);
    io->out(io->context,(uint16_t)(base+3u),3); /* 8N1 */
    io->out(io->context,(uint16_t)(base+2u),0xc7); /* FIFO reset */
    io->out(io->context,(uint16_t)(base+4u),3); /* DTR RTS */
    u->active=1;return 1;
}
static int byte_out(struct port_serial *u,
                    const struct port_serial_io *io,uint8_t value) {
    for(unsigned i=0;i<PORT_SERIAL_WAIT_LIMIT;i++) {
        if(io->in(io->context,(uint16_t)(u->base+5u))&0x20u) {
            io->out(io->context,u->base,value);++u->sent;return 1;
        }
    }
    ++u->dropped;return 0;
}
int port_serial_putc(struct port_serial *u,
                      const struct port_serial_io *io,char c) {
    if(!u||!callbacks(io)||!u->active)return -1;
    if(c=='\n') (void)byte_out(u,io,'\r');
    return byte_out(u,io,(uint8_t)c);
}

/* A polled receive path: no UART IRQ dependency. Return -1 when empty;
 * consume damaged bytes rather than dispatching malformed commands. */
int port_serial_getc_raw(struct port_serial *u,const struct port_serial_io *io) {
    if(!u||!u->active||!callbacks(io))return -1;
    uint8_t status=io->in(io->context,(uint16_t)(u->base+5u));
    if(!(status&0x01u))return -1;
    uint8_t value=io->in(io->context,u->base);
    if(status&0x1eu){++u->rx_errors;return -1;}
    ++u->received;
    if(value=='\r'){u->last_cr=1;return '\n';}
    if(value=='\n' && u->last_cr){u->last_cr=0;return -1;}
    u->last_cr=0;
    if(value=='\n')return '\n';
    if(value==0x7fu||value==0x08u)return '\b';
    if(value>=' '&&value<='~')return (int)value;
    return -1;
}

int port_serial_getc(struct port_serial *u,const struct port_serial_io *io) {
    int value=port_serial_getc_raw(u,io);
    if(value>='a'&&value<='z')return value-'a'+'A';
    return value;
}
