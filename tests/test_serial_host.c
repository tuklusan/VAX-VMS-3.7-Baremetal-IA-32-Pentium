#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include "vms_serial.h"
struct io_state {uint8_t lsr,rx,bytes[32];unsigned n,reads,wait;};
static uint8_t rd(void *x,uint16_t port) {
    struct io_state *s=x;
    ++s->reads;
    if(port==PORT_SERIAL_COM1+5u) {
        if(s->wait){--s->wait;return 0;}
        return s->lsr;
    }
    if(port==PORT_SERIAL_COM1)return s->rx;
    return 0;
}
static void wr(void *x,uint16_t port,uint8_t v) {
    struct io_state *s=x;
    if(port==PORT_SERIAL_COM1&&s->n<sizeof s->bytes)s->bytes[s->n++]=v;
}
int main(void) {
    struct port_serial u={0};struct io_state io={.lsr=0x60};
    struct port_serial_io v={rd,wr,&io};
    assert(port_serial_setup(&u,&v,PORT_SERIAL_COM1)==1&&u.active);
    io.n=0; /* discard setup writes */
    assert(port_serial_putc(&u,&v,'X')==1);
    assert(port_serial_putc(&u,&v,'\n')==1);
    assert(io.n==3&&io.bytes[0]=='X'&&io.bytes[1]=='\r'&&io.bytes[2]=='\n');
    assert(u.sent==3&&!u.dropped);
    io.lsr=0;io.wait=0;
    assert(port_serial_putc(&u,&v,'Z')==0&&u.dropped==1);
    io.lsr=0x20;io.wait=3;
    assert(port_serial_putc(&u,&v,'A')==1);
    io.lsr=0x61;io.rx='q';
    assert(port_serial_getc_raw(&u,&v)=='q');
    assert(port_serial_getc(&u,&v)=='Q');
    io.lsr=0xff;
    assert(port_serial_setup(&u,&v,PORT_SERIAL_COM1)==0&&!u.active);
    assert(port_serial_putc(&u,&v,'B')<0);
    assert(port_serial_setup(0,&v,PORT_SERIAL_COM1)<0);
    assert(port_serial_setup(&u,&v,0)<0);
    assert(port_serial_setup(&u,&v,65529)<0);
    puts("M23 UART serial output, CRLF, timeout and absent-port PASS");
    return 0;
}
