#include "vms_serial.h"
static struct port_serial com1;
static uint8_t uart_in(void *ctx,uint16_t port) {
    (void)ctx;uint8_t v;
    __asm__ volatile("inb %1,%0":"=a"(v):"Nd"(port));
    return v;
}
static void uart_out(void *ctx,uint16_t port,uint8_t v) {
    (void)ctx;
    __asm__ volatile("outb %0,%1"::"a"(v),"Nd"(port));
}
static const struct port_serial_io serial_io={uart_in,uart_out,0};
int port_serial_x86_init(void) {
    return port_serial_setup(&com1,&serial_io,PORT_SERIAL_COM1);
}
int port_serial_x86_getc_raw(void) {
    return port_serial_getc_raw(&com1,&serial_io);
}
int port_serial_x86_getc(void) {
    return port_serial_getc(&com1,&serial_io);
}
void port_serial_x86_putc(char c) {
    if(com1.active)(void)port_serial_putc(&com1,&serial_io,c);
}
uint32_t port_serial_x86_sent(void){return com1.sent;}
uint32_t port_serial_x86_dropped(void){return com1.dropped;}
int port_serial_x86_active(void){return com1.active;}
