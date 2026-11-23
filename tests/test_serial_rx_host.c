#include "vms_serial.h"
#include <assert.h>
#include <stdio.h>
#include <stdint.h>
struct fixture { uint8_t status, byte; unsigned reads; };
static uint8_t rd(void *context, uint16_t port) {
    struct fixture *f=context;
    ++f->reads;
    if(port==PORT_SERIAL_COM1+5u)return f->status;
    if(port==PORT_SERIAL_COM1)return f->byte;
    return 0;
}
static void wr(void *context,uint16_t port,uint8_t byte){
    (void)context;(void)port;(void)byte;
}
int main(void) {
    struct fixture fixture={.status=0x60u};
    const struct port_serial_io io={rd,wr,&fixture};
    struct port_serial uart={0};
    assert(port_serial_setup(&uart,&io,PORT_SERIAL_COM1)==1);
    assert(port_serial_getc(&uart,&io)==-1);
    assert(fixture.reads==2u);
    fixture.status=0x61u;
    fixture.byte='x';
    assert(port_serial_getc(&uart,&io)=='X');
    fixture.byte='N';assert(port_serial_getc(&uart,&io)=='N');
    fixture.byte='\r';assert(port_serial_getc(&uart,&io)=='\n');
    fixture.byte='\n';assert(port_serial_getc(&uart,&io)==-1);
    fixture.byte='\n';assert(port_serial_getc(&uart,&io)=='\n');
    fixture.byte=127;assert(port_serial_getc(&uart,&io)=='\b');
    fixture.byte=8;assert(port_serial_getc(&uart,&io)=='\b');
    fixture.byte=0x07;assert(port_serial_getc(&uart,&io)==-1);
    assert(uart.received==8u && !uart.rx_errors);
    fixture.status=0x63;fixture.byte='A';
    assert(port_serial_getc(&uart,&io)==-1 && uart.rx_errors==1u);
    assert(port_serial_getc(0,&io)==-1);
    assert(port_serial_getc(&uart,0)==-1);
    uart.active=0; assert(port_serial_getc(&uart,&io)==-1);
    puts("M36 serial COM1 input: ASCII, CRLF, erase, errors, no byte PASS");
    return 0;
}
