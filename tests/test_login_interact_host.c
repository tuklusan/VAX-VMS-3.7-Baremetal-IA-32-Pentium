#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include "vms_login_interact.h"

#define CHECK(x) do { if(!(x)){fprintf(stderr,"LOGIN INTERACT %d\n",__LINE__);return 1;} }while(0)
struct fixture {
    int mode, disabled, wrong, unknown, captive, use_notice;
    unsigned username_reads, password_reads, noecho_reads, open_count,
             close_count, output_count, auto_count, priv_enable, priv_disable;
    unsigned list_calls, last_input_flags;
    char output[2048];
    char current_username[32], current_terminal[64];
};
static int read_sector(void *c,uint32_t lba,uint16_t words[256]){
    FILE *f=c;
    uint8_t bytes[512];
    if(fseek(f,(long)lba*512L,SEEK_SET)||fread(bytes,1,512,f)!=512)return -1;
    for(unsigned i=0;i<256;i++)words[i]=(uint16_t)(bytes[2*i] | ((uint16_t)bytes[2*i+1]<<8));
    return 0;
}
static int open_file(void *ctx,unsigned kind,const char *spec,struct vms_lgi_stream *s){
    struct fixture *f=ctx;(void)spec;
    f->open_count++;
    s->ifi=(uint16_t)(kind+40u);s->isi=(uint16_t)(kind+60u);
    s->channel=(uint16_t)(kind+70u);s->readable=1;
    strcpy(s->device,"TTA0");strcpy(s->resolved,"TTA0:LOGIN.LOG");
    return VMS_LGI_OK;
}
static int connect_file(void *ctx,struct vms_lgi_stream *s){(void)ctx;(void)s;return VMS_LGI_OK;}
static int close_file(void *ctx,struct vms_lgi_stream *s){
    struct fixture *f=ctx;(void)s;f->close_count++;return VMS_LGI_OK;
}
static int write_file(void *ctx,struct vms_lgi_stream *s,const uint8_t *v,unsigned n){
    struct fixture *f=ctx;(void)s;
    if(f->output_count+n>=sizeof f->output)return VMS_LGI_NOSPACE;
    memcpy(f->output+f->output_count,v,n);f->output_count+=n;
    f->output[f->output_count]=0;
    return VMS_LGI_OK;
}
static int get_file(void *ctx,struct vms_lgi_stream *s,uint8_t *buf,
                    unsigned cap,unsigned flags,unsigned timeout,
                    const char *prompt,unsigned prompt_len){
    struct fixture *f=ctx;(void)timeout;(void)prompt_len;
    if(s->ifi==43){
        if(f->list_calls++>0)return VMS_LGI_EOF;
        if(cap<4)return VMS_LGI_NOSPACE;
        memcpy(buf,"MOTD",4);return 4;
    }
    const char *text;
    f->last_input_flags=flags;
    if(prompt && strstr(prompt,"assword")){
        f->password_reads++;if(flags&VMS_LGI_READ_QUIET)f->noecho_reads++;
        text=f->wrong?"WRONG":"PASSWORD";
    }else{
        f->username_reads++;
        text=f->unknown?"UNLISTED":f->captive?"LABUSER/CLI=DCL":"LABUSER";
    }
    unsigned n=(unsigned)strlen(text);
    if(n>cap)return VMS_LGI_NOSPACE;
    memcpy(buf,text,n);
    return (int)n;
}
static int set_logical(void *ctx,const char *name,const uint8_t *buf,
                       unsigned n,unsigned exec){
    (void)ctx;(void)name;(void)buf;
    return n>=5u && exec?VMS_LGI_OK:VMS_LGI_ARGUMENT;
}
static int terminal(void *ctx,char *out,unsigned cap){
    (void)ctx; if(cap<8)return -1;
    strcpy(out,"__TTA0:");return 7;
}
static int set_name(void *ctx,const char *name){
    struct fixture *f=ctx;snprintf(f->current_username,sizeof f->current_username,"%s",name);
    return 1;
}
static int set_device(void *ctx,const char *name){
    struct fixture *f=ctx;snprintf(f->current_terminal,sizeof f->current_terminal,"%s",name);
    return 1;
}
static int privilege(void *ctx,int enable){
    struct fixture *f=ctx;if(enable)f->priv_enable++;else f->priv_disable++;
    return 1;
}
static int auto_user(void *ctx,const char *terminal,char *out,unsigned cap){
    struct fixture *f=ctx;f->auto_count++;
    if(strcmp(terminal,"_TTA0:")!=0)return -1;
    if(!f->mode)return 0;
    if(cap<8)return -1;
    strcpy(out,"labuser");return 7;
}
static int notice(void *ctx,const char *name,uint8_t *buf,unsigned cap){
    struct fixture *f=ctx;
    if(!f->use_notice || strcmp(name,"SYS$WELCOME")!=0)return -1;
    const char *message=f->use_notice==1?"custom welcome":"@MOTD.LIS";
    unsigned n=(unsigned)strlen(message);
    if(n>=cap)return -1;
    memcpy(buf,message,n);return (int)n;
}
static int node(void *ctx,char *out,unsigned cap){
    (void)ctx;if(cap<6)return -1;
    strcpy(out,"NODE1");return 5;
}
static uint32_t user_flags(void *ctx,const struct port_uaf_record *user){
    struct fixture *f=ctx;(void)user;return (uint32_t)f->disabled;
}
static unsigned mail(void *ctx,const struct port_uaf_record *user){
    (void)ctx;(void)user;return 2;
}
static int flush_file(void *ctx,struct vms_lgi_stream *s){(void)ctx;(void)s;return VMS_LGI_OK;}
static const struct vms_lgi_ops file_ops={
    .open=open_file,.connect=connect_file,.close=close_file,.read=get_file,
    .write=write_file,.logical=set_logical,.flush=flush_file,.display=flush_file
};
static const struct vms_lgi_interact_ops interaction_ops={
    .terminal_name=terminal,.set_username=set_name,.set_termname=set_device,
    .system_privilege=privilege,.auto_user=auto_user,.translate=notice,
    .node_name=node,.uaf_flags=user_flags,.mail_count=mail
};
static int setup(struct fixture *f,struct vms_lgi_fileio *fileio,
                 struct vms_lgi_interact *s,struct port_uaf_service *auth){
    vms_lgi_fileio_init(fileio,&file_ops,f);
    vms_lgi_interact_init(s,fileio,auth,&interaction_ops,f);
    return 1;
}
int main(void){
    FILE *image=fopen("portdisk.img","rb");CHECK(image);
    struct port_uaf_disk disk;
    port_uaf_disk_init(&disk,read_sector,image,3);
    struct port_uaf_service auth=port_uaf_disk_service(&disk);
    struct fixture f={0};struct vms_lgi_fileio io;
    struct vms_lgi_interact session;
    CHECK(setup(&f,&io,&session,&auth));
    CHECK(vms_lgi_init_interactive(&session)==VMS_LOGIN_OK);
    CHECK(session.logged_in && !session.auto_selected && session.user_present);
    CHECK(f.username_reads==1&&f.password_reads==1&&f.noecho_reads==1);
    CHECK(strcmp(f.current_username,"LABUSER")==0&&strcmp(f.current_terminal,"__TTA0:")==0);
    CHECK(vms_lgi_announce(&session,"3.7")==VMS_LOGIN_OK);
    CHECK(strstr(f.output,"Welcome to VAX/VMS version 3.7 on node NODE1")!=NULL);
    CHECK(strstr(f.output,"You have 2 new Mail messages.")!=NULL);
    CHECK(vms_lgi_close_input(&io)==VMS_LGI_OK);
    CHECK(vms_lgi_close_output(&io)==VMS_LGI_OK);

    f=(struct fixture){.wrong=1};setup(&f,&io,&session,&auth);
    CHECK(vms_lgi_init_interactive(&session)==VMS_LOGIN_AUTH);
    CHECK(!session.logged_in && f.password_reads==1);
    f=(struct fixture){.unknown=1};setup(&f,&io,&session,&auth);
    CHECK(vms_lgi_init_interactive(&session)==VMS_LOGIN_AUTH);
    CHECK(!session.logged_in && f.password_reads==1);
    f=(struct fixture){.disabled=VMS_LOGIN_FLAG_DISABLED};setup(&f,&io,&session,&auth);
    CHECK(vms_lgi_init_interactive(&session)==VMS_LOGIN_DISABLED);
    CHECK(!session.logged_in);
    f=(struct fixture){.disabled=VMS_LOGIN_FLAG_CAPTIVE,.captive=1};setup(&f,&io,&session,&auth);
    CHECK(vms_lgi_init_interactive(&session)==VMS_LOGIN_CAPTIVE);
    CHECK(!session.logged_in&&strcmp(session.command.cli,"SYS$SYSTEM:DCL")==0);

    f=(struct fixture){.mode=1};setup(&f,&io,&session,&auth);
    CHECK(vms_lgi_init_interactive(&session)==VMS_LOGIN_OK);
    CHECK(session.logged_in && session.auto_selected);
    CHECK(f.username_reads==0 && f.password_reads==1);
    CHECK(f.priv_enable==1 && f.priv_disable==1 && f.auto_count==1);
    f=(struct fixture){.use_notice=1};setup(&f,&io,&session,&auth);
    CHECK(vms_lgi_init_interactive(&session)==VMS_LOGIN_OK);
    CHECK(vms_lgi_announce(&session,"3.7")==VMS_LOGIN_OK);
    CHECK(strstr(f.output,"custom welcome")!=NULL);
    f=(struct fixture){.use_notice=2};setup(&f,&io,&session,&auth);
    CHECK(vms_lgi_init_interactive(&session)==VMS_LOGIN_OK);
    CHECK(vms_lgi_announce(&session,"3.7")==VMS_LOGIN_OK);
    CHECK(strstr(f.output,"MOTD")!=NULL && f.list_calls==2);
    uint8_t password[24];memset(password,'X',sizeof password);
    vms_lgi_zero_password(password,sizeof password);
    for(unsigned i=0;i<sizeof password;i++)CHECK(password[i]==0);
    fclose(image);
    puts("LOGIN INTERACT live UAF, command, terminal, password, auto user and welcome PASS");
    return 0;
}
