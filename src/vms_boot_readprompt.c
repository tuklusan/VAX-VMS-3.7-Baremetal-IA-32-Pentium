#include "vms_boot_readprompt.h"

void port_boo_console_init(struct port_boo_console *console,
                           const struct port_boo_console_io *io,
                           void *context) {
    if (!console) return;
    *console = (struct port_boo_console){0};
    console->io = io;
    console->context = context;
}

uint32_t port_boo_readprompt(struct port_boo_console *console,
                             const char *prompt, unsigned max_length,
                             uint8_t *counted_buffer) {
    unsigned length=0;
    if (!console || !console->io || !prompt || max_length>255u ||
        (max_length && !counted_buffer)) return PORT_BOO_BADPARAM;
    while (prompt[length] && length<65535u) length++;
    if (prompt[length]) return PORT_BOO_BADPARAM;
    if (!console->channel) {
        if (!console->io->assign ||
            !(console->io->assign(console->context,PORT_BOO_CONSOLE_DEVICE,3u,
                                  &console->channel)&1u) ||
            !console->channel) return PORT_BOO_BADPARAM;
    }
    console->iosb=0;
    if (max_length) {
        uint16_t received=0;
        if (!console->io->readprompt) return PORT_BOO_BADPARAM;
        uint32_t status=console->io->readprompt(console->context,console->channel,
                    (char *)(counted_buffer+1u),max_length,prompt,length,
                    &console->iosb,&received);
        if (!(status&1u)) return status;
        if (received>max_length) return PORT_BOO_BADPARAM;
        counted_buffer[0]=(uint8_t)received;
        return console->iosb;
    }
    if (!console->io->write) return PORT_BOO_BADPARAM;
    uint32_t status=console->io->write(console->context,console->channel,
                                       prompt,length,&console->iosb);
    return (status&1u)?console->iosb:status;
}

uint32_t port_rio_output_line(struct port_boo_console *console) {
    if (!console || console->outlen>PORT_BOO_OUTBUF_SIZE-3u)
        return PORT_BOO_BADPARAM;
    unsigned length=console->outlen;
    console->outbuf[length]='\r';
    console->outbuf[length+1u]='\n';
    console->outbuf[length+2u]=0;
    return port_boo_readprompt(console,console->outbuf,0,0);
}
