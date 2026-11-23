#ifndef PORTLAB_SERIAL_H
#define PORTLAB_SERIAL_H
#include <stdint.h>
/* Bounded 16550/8250-compatible UART output to allow DOSBox-X capture. */
#define PORT_SERIAL_COM1 0x3f8u
#define PORT_SERIAL_WAIT_LIMIT 1024u
struct port_serial_io {
    uint8_t (*in)(void *context,uint16_t port);
    void (*out)(void *context,uint16_t port,uint8_t value);
    void *context;
};
struct port_serial {
    uint16_t base;
    uint8_t active, last_cr;
    uint32_t sent, dropped, received, rx_errors;
};
int port_serial_setup(struct port_serial *uart,
                      const struct port_serial_io *io,uint16_t base);
int port_serial_putc(struct port_serial *uart,
                      const struct port_serial_io *io,char c);
int port_serial_getc(struct port_serial *uart, const struct port_serial_io *io);
int port_serial_getc_raw(struct port_serial *uart, const struct port_serial_io *io);
int port_serial_x86_getc(void);
int port_serial_x86_getc_raw(void);
int port_serial_x86_init(void);
void port_serial_x86_putc(char c);
uint32_t port_serial_x86_sent(void);
uint32_t port_serial_x86_dropped(void);
int port_serial_x86_active(void);
#endif
