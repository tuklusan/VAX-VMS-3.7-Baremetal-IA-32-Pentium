#ifndef VMS_BOOT_READPROMPT_H
#define VMS_BOOT_READPROMPT_H

#include <stdint.h>

#define PORT_BOO_OUTBUF_SIZE 256u
#define PORT_BOO_CONSOLE_DEVICE "_OPA0"
#define PORT_BOO_NORMAL 1u
#define PORT_BOO_BADPARAM 0u

struct port_boo_console_io {
    uint32_t (*assign)(void *ctx, const char *device, unsigned mode,
                       uint16_t *channel);
    uint32_t (*write)(void *ctx, uint16_t channel, const char *text,
                      unsigned length, uint16_t *iosb);
    uint32_t (*readprompt)(void *ctx, uint16_t channel, char *buffer,
                           unsigned max_length, const char *prompt,
                           unsigned prompt_length, uint16_t *iosb,
                           uint16_t *received);
};

struct port_boo_console {
    const struct port_boo_console_io *io;
    void *context;
    uint16_t channel;
    uint16_t iosb;
    uint16_t outlen;
    char outbuf[PORT_BOO_OUTBUF_SIZE];
};

void port_boo_console_init(struct port_boo_console *console,
                           const struct port_boo_console_io *io,
                           void *context);
uint32_t port_boo_readprompt(struct port_boo_console *console,
                             const char *prompt, unsigned max_length,
                             uint8_t *counted_buffer);
uint32_t port_rio_output_line(struct port_boo_console *console);

#endif
