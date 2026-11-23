#include "vms_login_fileio.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <stdint.h>

struct fixture {
    unsigned opens[4], connects, closes, flushes, displays, logicals;
    unsigned arm, disarm, cancel, puts, chars, get_calls, list_calls, quiet;
    unsigned last_flags, last_timeout, last_prompt;
    int force_connect_error;
    uint8_t output[256];
};
static int open_fn(void *ctx,unsigned kind,const char *name,struct vms_lgi_stream *s){
    struct fixture *f=ctx;f->opens[kind]++;
    assert((name && name[0]=='S') || kind==VMS_LGI_LIST);
    s->ifi=(uint16_t)(kind+40);
    s->isi=(uint16_t)(kind+50);
    s->channel=(uint16_t)(kind+60);
    s->readable=kind==VMS_LGI_OUTPUT;
    s->concealed=kind==VMS_LGI_OUTPUT;
    strcpy(s->device,"TT0");
    strcpy(s->resolved,"DISK:[LOGIN]LOGIN.LOG;1");
    return VMS_LGI_OK;
}
static int connect_fn(void *ctx,struct vms_lgi_stream *s){
    struct fixture *f=ctx;f->connects++;(void)s;
    return f->force_connect_error?VMS_LGI_IO:VMS_LGI_OK;
}
static int close_fn(void *ctx,struct vms_lgi_stream *s){
    struct fixture *f=ctx;f->closes++;(void)s;return VMS_LGI_OK;
}
static int flush_fn(void *ctx,struct vms_lgi_stream *s){
    struct fixture *f=ctx;f->flushes++;(void)s;return VMS_LGI_OK;
}
static int display_fn(void *ctx,struct vms_lgi_stream *s){
    struct fixture *f=ctx;f->displays++;(void)s;return VMS_LGI_OK;
}
static int logical_fn(void *ctx,const char *name,const uint8_t *v,
                      unsigned n,unsigned mode){
    struct fixture *f=ctx;f->logicals++;
    assert(mode==1 && n>=7 && v[0]==27 && v[1]==0);
    assert(strncmp(name,"SYS$",4)==0);
    if(f->logicals<3)assert(v[2]==42u && v[3]==0);
    return VMS_LGI_OK;
}
static int write_fn(void *ctx,struct vms_lgi_stream *s,const uint8_t *data,unsigned n){
    struct fixture *f=ctx;(void)s;
    assert(f->chars+n<sizeof f->output);
    memcpy(f->output+f->chars,data,n);
    f->chars+=n;f->puts++;return VMS_LGI_OK;
}
static int read_fn(void *ctx,struct vms_lgi_stream *s,uint8_t *dest,
                   unsigned cap,unsigned flags,unsigned timeout,
                   const char *prompt,unsigned promptlen){
    struct fixture *f=ctx;f->get_calls++;
    if(s->ifi==43){
        if(f->list_calls++==0){assert(cap==128);memcpy(dest,"TITLE",5);return 5;}
        if(f->list_calls==3)return VMS_LGI_EOF;
        return 0; /* A blank record must not end a sequential file. */
    }
    f->last_flags=flags;f->last_timeout=timeout;f->last_prompt=promptlen;
    assert(prompt && memcmp(prompt,"Username: ",10)==0);
    assert(cap>=5);
    memcpy(dest,"alice",5);return 5;
}
static int format_fn(void *ctx,const uint8_t *ctl,unsigned n,
                     const uint32_t *args,unsigned ac,uint8_t *out,unsigned cap){
    (void)ctx;
    assert(n==3 && memcmp(ctl,"!UL",3)==0 && ac==1 && args[0]==7 && cap>=1);
    out[0]='7';return 1;
}
static int arm_fn(void *ctx,unsigned secs,unsigned id){
    struct fixture *f=ctx;assert(secs==30&&id==99);f->arm++;return VMS_LGI_OK;
}
static int disarm_fn(void *ctx,unsigned id){
    struct fixture *f=ctx;assert(id==99);f->disarm++;return VMS_LGI_OK;
}
static int cancel_fn(void *ctx,uint16_t channel){
    struct fixture *f=ctx;assert(channel==62);f->cancel++;return VMS_LGI_OK;
}
int main(void){
    struct fixture f={0};
    const struct vms_lgi_ops ops={.open=open_fn,.connect=connect_fn,
        .close=close_fn,.flush=flush_fn,.display=display_fn,.read=read_fn,
        .write=write_fn,.logical=logical_fn,.format=format_fn,
        .arm_timer=arm_fn,.cancel_timer=disarm_fn,.cancel_io=cancel_fn};
    struct vms_lgi_fileio io;
    vms_lgi_fileio_init(&io,&ops,&f);
    assert(vms_lgi_write_output(&io,(const uint8_t*)"SKIP",4)==0 && f.puts==0);
    assert(vms_lgi_open_output(&io)==VMS_LGI_OK);
    assert(io.output_ifi==42 && io.output_isi==52 && io.output_characteristics==0);
    assert(strcmp((const char*)io.output_filename,"LOGIN.LOG;1")==0);
    assert(io.output_dvi[0]==27 && io.output_dvi[1]=='T');
    assert(io.output_logical_len==9 && io.output_logical[4]==27 && io.output_logical[5]=='T');
    assert(vms_lgi_open_input(&io)==VMS_LGI_OK && io.input_shared==1);
    assert(f.opens[VMS_LGI_INPUT]==0 && f.logicals==4);
    assert(io.input_channel==io.output.channel && io.input_ifi==io.output.ifi);
    uint8_t buf[32];
    assert(vms_lgi_get_input(&io,buf,sizeof buf)==VMS_LGI_OK);
    assert(strcmp((const char*)buf,"ALICE")==0 && io.last_input_length==5);
    assert(f.last_flags==(VMS_LGI_READ_PROMPT|VMS_LGI_READ_UPPER|
        VMS_LGI_READ_TIMEOUT|VMS_LGI_READ_PURGE) && f.last_timeout==15 && f.last_prompt==10);
    assert(vms_lgi_write_output(&io,(const uint8_t*)"OK",2)==0);
    assert(f.puts==1 && f.arm==1 && f.disarm==1);
    const uint8_t fao[]={3,'!','U','L'};const uint32_t args[]={7};
    assert(vms_lgi_write_fao(&io,fao,args,1)==0);
    assert(vms_lgi_write_file(&io,"SYSTEM.LIS")==VMS_LGI_OK);
    assert(f.list_calls==3 && f.opens[VMS_LGI_LIST]==1);
    assert(f.puts==4 && f.chars==8 && memcmp(f.output,"OK7TITLE",8)==0);
    assert(vms_lgi_write_timeout(&io)==VMS_LGI_OK && f.cancel==1);
    assert(vms_lgi_close_input(&io)==VMS_LGI_OK && f.closes==1);
    assert(vms_lgi_close_output(&io)==VMS_LGI_OK);
    assert(f.closes==2 && f.flushes==1 && f.displays==1);
    assert(vms_lgi_get_input(&io,buf,sizeof buf)==VMS_LGI_CLOSED);
    assert(vms_lgi_write_output(&io,(const uint8_t*)"Z",1)==0 && f.puts==4);
    vms_lgi_fileio_init(&io,&ops,&f);
    f.force_connect_error=1;
    assert(vms_lgi_open_output(&io)==VMS_LGI_IO);
    assert(!io.output_open && f.closes==3);
    f.force_connect_error=0;
    assert(vms_lgi_open_input(&io)==VMS_LGI_OK);
    assert(!io.input_shared && io.input_channel==61);
    assert(vms_lgi_close_input(&io)==VMS_LGI_OK);
    assert(f.closes==4);
    assert(vms_lgi_get_input(&io,buf,0)==VMS_LGI_ARGUMENT);
    puts("LOGIN FILEIO stream/open/close, names, records, FAO, timer, cancel PASS");
    return 0;
}
