#include "vms_login_uart.h"
#include "vms_serial.h"

static int eq(const char *a,const char *b){
    for(;*a && *b; a++,b++)if(*a!=*b)return 0;
    return !*a && !*b;
}
static int serial_open(void *ctx,unsigned which,const char *spec,
                       struct vms_lgi_stream *stream) {
    (void)ctx; (void)spec;
    if(!port_serial_x86_active() || which==VMS_LGI_LIST)return VMS_LGI_IO;
    stream->ifi=37u;
    stream->isi=1u;
    stream->channel=1u;
    stream->characteristics=0x20u;
    stream->readable=1u;
    stream->concealed=0;
    const char device[]="TTA0";
    const char resolved[]="TTA0:LOGIN.LOG";
    for(unsigned i=0;i<sizeof device;i++)stream->device[i]=device[i];
    for(unsigned i=0;i<sizeof resolved;i++)stream->resolved[i]=resolved[i];
    return VMS_LGI_OK;
}
static int serial_connect(void *ctx,struct vms_lgi_stream *stream){
    (void)ctx;(void)stream;
    return port_serial_x86_active()?VMS_LGI_OK:VMS_LGI_IO;
}
static int serial_close(void *ctx,struct vms_lgi_stream *stream){
    (void)ctx;(void)stream;return VMS_LGI_OK;
}
static int serial_write(void *ctx,struct vms_lgi_stream *stream,
                        const uint8_t *buf,unsigned length) {
    struct vms_lgi_uart *io=ctx;
    (void)stream;
    if(!port_serial_x86_active())return VMS_LGI_IO;
    uint32_t before=port_serial_x86_dropped();
    for(unsigned i=0;i<length;i++) {
        if(io->timer_active && io->ticks &&
            (uint32_t)(io->ticks()-io->arm_at)>=3000u) {
            io->canceled++;
            return VMS_LGI_IO;
        }
        port_serial_x86_putc((char)buf[i]);
    }
    if(port_serial_x86_dropped()!=before)return VMS_LGI_IO;
    io->output_count+=length;
    return VMS_LGI_OK;
}
static int serial_read(void *ctx,struct vms_lgi_stream *stream,uint8_t *buf,
                       unsigned cap,unsigned flags,unsigned timeout,
                       const char *prompt,unsigned prompt_length) {
    struct vms_lgi_uart *io=ctx;
    (void)stream;
    if(flags & VMS_LGI_READ_PURGE)
        while(port_serial_x86_getc()>=0){}
    if((flags & VMS_LGI_READ_PROMPT) && prompt)
        for(unsigned i=0;i<prompt_length;i++)port_serial_x86_putc(prompt[i]);
    if(!io->ticks)return VMS_LGI_SERVICE;
    uint32_t last=io->ticks();
    unsigned count=0;
    for(;;) {
        int c=port_serial_x86_getc();
        if(c<0){
            if((flags & VMS_LGI_READ_TIMEOUT)&&
               (uint32_t)(io->ticks()-last)>=(uint32_t)(timeout*100u))
                return VMS_LGI_IO;
            continue;
        }
        last=io->ticks();
        if(c=='\n') { io->input_count++; return (int)count; }
        if(c=='\b'){if(count)count--;continue;}
        if(count>=cap)return VMS_LGI_NOSPACE;
        buf[count++]=(uint8_t)c;
        if(!(flags & VMS_LGI_READ_QUIET))port_serial_x86_putc((char)c);
    }
}
static int serial_logical(void *ctx,const char *name,const uint8_t *v,
                          unsigned length,unsigned exec) {
    struct vms_lgi_uart *io=ctx;
    unsigned slot;
    if(eq(name,"SYS$INPUT"))slot=0;
    else if(eq(name,"SYS$COMMAND"))slot=1;
    else if(eq(name,"SYS$OUTPUT"))slot=2;
    else if(eq(name,"SYS$ERROR"))slot=3;
    else return VMS_LGI_ARGUMENT;
    if(!exec||length>sizeof io->logical[0]||length<5||v[0]!=27)
        return VMS_LGI_ARGUMENT;
    for(unsigned i=0;i<length;i++)io->logical[slot][i]=v[i];
    io->length[slot]=length;
    return VMS_LGI_OK;
}
static int serial_flush(void *ctx,struct vms_lgi_stream *stream){
    (void)ctx;(void)stream;return VMS_LGI_OK;
}
static int serial_display(void *ctx,struct vms_lgi_stream *stream){
    (void)ctx;(void)stream;return VMS_LGI_OK;
}
static int serial_arm(void *ctx,unsigned seconds,unsigned request_id){
    struct vms_lgi_uart *io=ctx;
    if(seconds!=30u||request_id!=VMS_LGI_TIMER_ID||!io->ticks)return VMS_LGI_ARGUMENT;
    io->arm_at=io->ticks();
    io->timer_id=request_id;io->timer_active=1;
    return VMS_LGI_OK;
}
static int serial_disarm(void *ctx,unsigned request_id){
    struct vms_lgi_uart *io=ctx;
    if(!io->timer_active||request_id!=io->timer_id)return VMS_LGI_ARGUMENT;
    io->timer_active=0u;
    return VMS_LGI_OK;
}
static int serial_cancel(void *ctx,uint16_t chan){
    struct vms_lgi_uart *io=ctx;
    if(chan!=1u)return VMS_LGI_ARGUMENT;
    io->canceled++;
    return VMS_LGI_OK;
}
static const struct vms_lgi_ops serial_ops={
    .open=serial_open,.connect=serial_connect,.close=serial_close,
    .flush=serial_flush,.display=serial_display,
    .read=serial_read,.write=serial_write,.logical=serial_logical,
    .arm_timer=serial_arm,.cancel_timer=serial_disarm,.cancel_io=serial_cancel
};
void vms_lgi_uart_init(struct vms_lgi_uart *io,uint32_t (*ticks)(void)){
    if(!io)return;
    *io=(struct vms_lgi_uart){0};
    io->ticks=ticks;
}
const struct vms_lgi_ops *vms_lgi_uart_ops(void){return &serial_ops;}
