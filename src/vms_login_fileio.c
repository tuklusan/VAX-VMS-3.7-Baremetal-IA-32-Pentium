#include "vms_login_fileio.h"

static void clear_bytes(void *p, unsigned n) {
    uint8_t *b=p; for(unsigned i=0;i<n;i++) b[i]=0;
}
static unsigned string_length(const char *s, unsigned limit) {
    unsigned i=0; if(!s)return 0;
    while(i<limit && s[i]) ++i;
    return i;
}
static int copy_bytes(uint8_t *dst, unsigned cap,
                      const uint8_t *src, unsigned size) {
    if(!dst||!src||size>cap)return VMS_LGI_NOSPACE;
    for(unsigned i=0;i<size;i++)dst[i]=src[i];
    return VMS_LGI_OK;
}
static int save_device(uint8_t *out, unsigned cap,
                       const struct vms_lgi_stream *stream) {
    unsigned n=string_length(stream->device,VMS_LGI_DEVICE_MAX);
    if(n==VMS_LGI_DEVICE_MAX || n+1u+(unsigned)stream->concealed>cap)
        return VMS_LGI_NOSPACE;
    /* The original NAM concealed-device name prepends a control byte. */
    unsigned index=0;
    if(stream->concealed)out[index++]=0x1bu;
    for(unsigned i=0;i<n;i++)out[index++]=(uint8_t)stream->device[i];
    out[index]=0;
    return VMS_LGI_OK;
}
static int make_logical(uint8_t *dest, unsigned *length,
                        const struct vms_lgi_stream *stream) {
    uint8_t device[VMS_LGI_DEVICE_MAX];
    if(save_device(device,sizeof device,stream)!=VMS_LGI_OK)
        return VMS_LGI_NOSPACE;
    unsigned n=string_length((const char*)device,sizeof device);
    if(n+5u>VMS_LGI_DEVICE_MAX+5u) return VMS_LGI_NOSPACE;
    dest[0]=27u;
    dest[1]=0u;
    dest[2]=(uint8_t)stream->ifi;
    dest[3]=(uint8_t)(stream->ifi>>8);
    for(unsigned i=0;i<n;i++)dest[4+i]=device[i];
    dest[4+n]=':';
    *length=5u+n;
    return VMS_LGI_OK;
}
static int logical_name(struct vms_lgi_fileio *io,const char *name,
                        const uint8_t *data,unsigned size) {
    if(!io->ops.logical) return VMS_LGI_SERVICE;
    return io->ops.logical(io->ctx,name,data,size,1u);
}
static int open_stream(struct vms_lgi_fileio *io,unsigned which,
                       const char *spec,struct vms_lgi_stream *s) {
    if(!io->ops.open || !io->ops.connect) return VMS_LGI_SERVICE;
    clear_bytes(s,sizeof *s);
    int rc=io->ops.open(io->ctx,which,spec,s);
    if(rc!=VMS_LGI_OK)return rc;
    rc=io->ops.connect(io->ctx,s);
    if(rc!=VMS_LGI_OK && io->ops.close)
        (void)io->ops.close(io->ctx,s);
    return rc;
}
void vms_lgi_fileio_init(struct vms_lgi_fileio *io,
                         const struct vms_lgi_ops *ops, void *ctx) {
    if(!io)return;
    clear_bytes(io,sizeof *io);
    if(ops)io->ops=*ops;
    io->ctx=ctx;
    io->input_prompt="Username: ";
    io->input_prompt_length=10u;
    io->input_flags=VMS_LGI_READ_PROMPT|VMS_LGI_READ_UPPER|VMS_LGI_READ_TIMEOUT|VMS_LGI_READ_PURGE;
    io->input_timeout=15u;
}
int vms_lgi_open_output(struct vms_lgi_fileio *io) {
    if(!io || io->output_open)return VMS_LGI_ARGUMENT;
    int rc=open_stream(io,VMS_LGI_OUTPUT,"SYS$OUTPUT",&io->output);
    if(rc!=VMS_LGI_OK)return io->last_status=rc;
    io->output_open=1u;
    io->output_ifi=io->output.ifi;
    io->output_isi=io->output.isi;
    io->output_characteristics=io->output.characteristics;
    rc=save_device(io->output_dvi,sizeof io->output_dvi,&io->output);
    if(rc!=VMS_LGI_OK)return io->last_status=rc;
    /* Only the final file name, not the full path, is held in LGI. */
    const char *name=io->output.resolved;
    unsigned length=string_length(name,sizeof io->output.resolved);
    if(length==sizeof io->output.resolved)return io->last_status=VMS_LGI_NOSPACE;
    unsigned start=0;
    for(unsigned i=0;i<length;i++)
        if(name[i]==']'||name[i]=='>'||name[i]==':')start=i+1u;
    if(copy_bytes(io->output_filename,sizeof io->output_filename-1u,
                  (const uint8_t*)(name+start),length-start)!=VMS_LGI_OK)
        return io->last_status=VMS_LGI_NOSPACE;
    io->output_filename[length-start]=0;
    rc=make_logical(io->output_logical,&io->output_logical_len,&io->output);
    if(rc==VMS_LGI_OK)
        rc=logical_name(io,"SYS$OUTPUT",io->output_logical,io->output_logical_len);
    if(rc==VMS_LGI_OK)
        rc=logical_name(io,"SYS$ERROR",io->output_logical,io->output_logical_len);
    return io->last_status=rc;
}
int vms_lgi_open_input(struct vms_lgi_fileio *io) {
    if(!io || io->input_open)return VMS_LGI_ARGUMENT;
    int rc;
    if(io->output_open && io->output.readable) {
        io->input=io->output;
        io->input_shared=1u;
        rc=VMS_LGI_OK;
    } else {
        io->input_shared=0;
        rc=open_stream(io,VMS_LGI_INPUT,"SYS$INPUT",&io->input);
    }
    if(rc!=VMS_LGI_OK)return io->last_status=rc;
    io->input_open=1u;
    io->input_channel=io->input.channel;
    io->input_ifi=io->input.ifi;
    io->input_isi=io->input.isi;
    io->input_characteristics=io->input.characteristics;
    rc=save_device(io->input_dvi,sizeof io->input_dvi,&io->input);
    if(rc==VMS_LGI_OK)
        rc=make_logical(io->input_logical,&io->input_logical_len,&io->input);
    if(rc==VMS_LGI_OK)
        rc=logical_name(io,"SYS$INPUT",io->input_logical,io->input_logical_len);
    if(rc==VMS_LGI_OK)
        rc=logical_name(io,"SYS$COMMAND",io->input_logical,io->input_logical_len);
    return io->last_status=rc;
}
int vms_lgi_close_input(struct vms_lgi_fileio *io) {
    if(!io)return VMS_LGI_ARGUMENT;
    if(!io->input_open)return VMS_LGI_OK;
    int rc=VMS_LGI_OK;
    if(!io->input_shared && io->ops.close)
        rc=io->ops.close(io->ctx,&io->input);
    io->input_open=0u;
    io->input_shared=0u;
    return io->last_status=rc;
}
int vms_lgi_close_output(struct vms_lgi_fileio *io) {
    if(!io)return VMS_LGI_ARGUMENT;
    if(!io->output_open)return VMS_LGI_OK;
    int rc=VMS_LGI_OK,tmp;
    if(io->ops.flush){tmp=io->ops.flush(io->ctx,&io->output);if(tmp!=VMS_LGI_OK)rc=tmp;}
    if(io->ops.display){tmp=io->ops.display(io->ctx,&io->output);if(tmp!=VMS_LGI_OK)rc=tmp;}
    if(io->ops.close){tmp=io->ops.close(io->ctx,&io->output);if(tmp!=VMS_LGI_OK)rc=tmp;}
    io->output_open=0u;
    if(io->input_shared)io->input_open=io->input_shared=0u;
    return io->last_status=rc;
}
int vms_lgi_write_output(struct vms_lgi_fileio *io,
                         const uint8_t *data,unsigned len) {
    /* $PUTMSG actions return zero even when no stream exists. */
    if(!io)return VMS_LGI_ARGUMENT;
    if(!io->output_open)return 0;
    if((!data && len)||!io->ops.write){io->last_status=VMS_LGI_ARGUMENT;return 0;}
    int armed=0;
    if(io->ops.arm_timer && io->ops.arm_timer(io->ctx,30,VMS_LGI_TIMER_ID)==VMS_LGI_OK)
        armed=1;
    io->last_status=io->ops.write(io->ctx,&io->output,data,len);
    if(armed && io->ops.cancel_timer)(void)io->ops.cancel_timer(io->ctx,VMS_LGI_TIMER_ID);
    return 0;
}
int vms_lgi_write_timeout(struct vms_lgi_fileio *io) {
    if(!io||!io->ops.cancel_io)return VMS_LGI_SERVICE;
    return io->last_status=io->ops.cancel_io(io->ctx,io->input_channel);
}
int vms_lgi_get_input(struct vms_lgi_fileio *io,uint8_t *buf,
                      unsigned capacity) {
    if(!io||!buf||!capacity)return VMS_LGI_ARGUMENT;
    if(!io->input_open||!io->ops.read)return io->last_status=VMS_LGI_CLOSED;
    int result=io->ops.read(io->ctx,&io->input,buf,capacity-1u,
                 io->input_flags,io->input_timeout,
                 io->input_prompt,io->input_prompt_length);
    if(result<0)return io->last_status=result;
    if((unsigned)result>=capacity)return io->last_status=VMS_LGI_NOSPACE;
    io->last_input_length=(uint16_t)result;
    for(unsigned i=0;i<(unsigned)result;i++)
        if(buf[i]>='a'&&buf[i]<='z')buf[i]=(uint8_t)(buf[i]-'a'+'A');
    buf[result]=0;
    return io->last_status=VMS_LGI_OK;
}
int vms_lgi_write_file(struct vms_lgi_fileio *io,const char *filespec) {
    if(!io||!filespec||!io->ops.open||!io->ops.connect||!io->ops.read)
        return VMS_LGI_ARGUMENT;
    struct vms_lgi_stream stream;
    int rc=open_stream(io,VMS_LGI_LIST,filespec,&stream);
    if(rc!=VMS_LGI_OK)return io->last_status=rc;
    uint8_t buf[VMS_LGI_RECORD_MAX];
    for(;;){
        int got=io->ops.read(io->ctx,&stream,buf,sizeof buf,0,0,0,0);
        if(got==VMS_LGI_EOF)break;
        if(got<0){rc=got;break;}
        if((unsigned)got>sizeof buf){rc=VMS_LGI_NOSPACE;break;}
        (void)vms_lgi_write_output(io,buf,(unsigned)got);
        if(io->last_status!=VMS_LGI_OK){rc=io->last_status;break;}
    }
    if(io->ops.close)(void)io->ops.close(io->ctx,&stream);
    return io->last_status=rc;
}
int vms_lgi_write_fao(struct vms_lgi_fileio *io,const uint8_t *ascic,
                      const uint32_t *args,unsigned arg_count) {
    if(!io||!ascic||!io->ops.format)return VMS_LGI_SERVICE;
    uint8_t buffer[VMS_LGI_RECORD_MAX];
    unsigned n=ascic[0];
    int length=io->ops.format(io->ctx,ascic+1,n,args,arg_count,
                              buffer,sizeof buffer);
    if(length<0)return io->last_status=length;
    if((unsigned)length>sizeof buffer)return io->last_status=VMS_LGI_NOSPACE;
    return vms_lgi_write_output(io,buffer,(unsigned)length);
}
