#include "vms_login_interact.h"

static void clear(void *where,unsigned count){
    uint8_t *d=where;
    for(unsigned i=0;i<count;i++)d[i]=0;
}
static unsigned len(const char *s){unsigned i=0;if(s)while(s[i])i++;return i;}
static int copy(char *dst,unsigned cap,const char *src){
    if(!dst||!src)return 0;
    unsigned n=len(src);
    if(n>=cap)return 0;
    for(unsigned i=0;i<n;i++)dst[i]=src[i];
    dst[n]=0;
    return 1;
}
static void upcase(char *a){
    while(*a){if(*a>='a'&&*a<='z')*a=(char)(*a-'a'+'A');a++;}
}
static int send(struct vms_lgi_interact *s,const char *message){
    (void)vms_lgi_write_output(s->streams,(const uint8_t*)message,len(message));
    return s->streams->last_status==VMS_LGI_OK;
}
static uint32_t flags(struct vms_lgi_interact *s){
    return s->ops.uaf_flags ? s->ops.uaf_flags(s->context,&s->user) :
           (uint32_t)s->user.reserved[0];
}
static int select_user(struct vms_lgi_interact *s,const char *name){
    if(!s->uaf)return VMS_LOGIN_FILE;
    s->user_present=0;
    unsigned n=len(name);
    if(!n||n>VMS_LOGIN_USER_MAX)return VMS_LOGIN_AUTH;
    int rc=port_uaf_searchuser(s->uaf,(const uint8_t*)name,(uint16_t)n,
                                &s->user,sizeof s->user);
    if(rc==PORT_UAF_NORMAL){s->user_present=1;return VMS_LOGIN_OK;}
    if(rc==PORT_UAF_INVUSR)return VMS_LOGIN_AUTH;
    return VMS_LOGIN_FILE;
}
static int output_blanks(struct vms_lgi_interact *s){
    return send(s,"\n");
}
void vms_lgi_zero_password(uint8_t *password,unsigned length){
    volatile uint8_t *secret=password;
    if(!password)return;
    for(unsigned i=0;i<length;i++)secret[i]=0;
}
void vms_lgi_interact_init(struct vms_lgi_interact *s,
                           struct vms_lgi_fileio *streams,
                           struct port_uaf_service *uaf,
                           const struct vms_lgi_interact_ops *ops,void *context){
    if(!s)return;
    clear(s,sizeof *s);
    s->streams=streams;s->uaf=uaf;s->context=context;
    if(ops)s->ops=*ops;
}
int vms_lgi_write_announcement(struct vms_lgi_interact *s,const char *logical){
    if(!s||!s->streams||!logical)return VMS_LOGIN_FILE;
    if(!s->ops.translate)return VMS_LOGIN_NOAUTO;
    uint8_t value[128];
    int n=s->ops.translate(s->context,logical,value,sizeof value);
    if(n<0)return VMS_LOGIN_NOAUTO;
    if(n>=128)return VMS_LOGIN_FILE;
    value[n]=0;
    if(n && value[0]=='@'){
        int rc=vms_lgi_write_file(s->streams,(const char*)value+1);
        return rc==VMS_LGI_OK?VMS_LOGIN_OK:VMS_LOGIN_FILE;
    }
    if(n){
        if(!output_blanks(s)||
           vms_lgi_write_output(s->streams,value,(unsigned)n)!=0 ||
           s->streams->last_status!=VMS_LGI_OK||!output_blanks(s))
            return VMS_LOGIN_FILE;
    }
    return VMS_LOGIN_OK;
}
int vms_lgi_announce(struct vms_lgi_interact *s,const char *version){
    if(!s||!s->streams)return VMS_LOGIN_FILE;
    if(s->user_present && !(flags(s)&VMS_LOGIN_FLAG_NOWELCOME)){
        int rc=vms_lgi_write_announcement(s,"SYS$WELCOME");
        if(rc==VMS_LOGIN_NOAUTO){
            if(!send(s,"\tWelcome to VAX/VMS version "))return VMS_LOGIN_FILE;
            if(!send(s,version?version:""))return VMS_LOGIN_FILE;
            if(s->ops.node_name){
                char node[40];
                if(s->ops.node_name(s->context,node,sizeof node)>0 && node[0]){
                    if(!send(s," on node ")||!send(s,node))return VMS_LOGIN_FILE;
                }
            }
            if(!send(s,"\n"))return VMS_LOGIN_FILE;
        } else if(rc!=VMS_LOGIN_OK)return rc;
    }
    if(s->user_present && !(flags(s)&VMS_LOGIN_FLAG_NOMAIL) &&
       s->ops.mail_count){
        unsigned mail=s->ops.mail_count(s->context,&s->user);
        if(mail){
            char digits[12];unsigned n=0;
            do{digits[n++]=(char)('0'+mail%10u);mail/=10u;}while(mail&&n<sizeof digits);
            if(!send(s,"You have "))return VMS_LOGIN_FILE;
            while(n){char ch[2]={digits[--n],0};if(!send(s,ch))return VMS_LOGIN_FILE;}
            if(!send(s," new Mail messages.\n"))return VMS_LOGIN_FILE;
        }
    }
    return VMS_LOGIN_OK;
}
int vms_lgi_get_password(struct vms_lgi_interact *s){
    if(!s||!s->streams)return VMS_LOGIN_FILE;
    if(s->user_present){
        unsigned nonzero=0;
        for(unsigned i=0;i<PORT_UAF_HASH_BYTES;i++)nonzero|=s->user.hash[i];
        if(!nonzero)return VMS_LOGIN_OK;
    }
    struct vms_lgi_fileio *io=s->streams;
    const char *old_prompt=io->input_prompt;
    unsigned old_len=io->input_prompt_length, old_flags=io->input_flags;
    unsigned old_timeout=io->input_timeout;
    int local=s->ops.local_echo?s->ops.local_echo(s->context):0;
    uint8_t password[24];
    clear(password,sizeof password);
    io->input_prompt=local?"\nPassword: ###############":"Password: ";
    io->input_prompt_length=len(io->input_prompt);
    io->input_flags=(old_flags|VMS_LGI_READ_QUIET|VMS_LGI_READ_TIMEOUT|
                     VMS_LGI_READ_PROMPT)&~VMS_LGI_READ_PURGE;
    io->input_timeout=15;
    int status=vms_lgi_get_input(io,password,sizeof password);
    unsigned count=io->last_input_length;
    io->input_prompt=old_prompt;
    io->input_prompt_length=old_len;
    io->input_flags=old_flags;
    io->input_timeout=old_timeout;
    if(local)(void)send(s,"\r               \r");
    if(status!=VMS_LGI_OK){
        vms_lgi_zero_password(password,sizeof password);
        return VMS_LOGIN_FILE;
    }
    int outcome=VMS_LOGIN_AUTH;
    if(s->user_present){
        int rc=port_uaf_validate(s->uaf,(const uint8_t*)s->username,
                      (uint16_t)len(s->username),password,(uint16_t)count,
                      &s->user,sizeof s->user);
        outcome=rc==PORT_UAF_NORMAL?VMS_LOGIN_OK:
                rc==PORT_UAF_INVPWD?VMS_LOGIN_AUTH:VMS_LOGIN_FILE;
    }
    vms_lgi_zero_password(password,sizeof password);
    return outcome;
}
int vms_lgi_auto_login(struct vms_lgi_interact *s){
    if(!s||!s->streams)return VMS_LOGIN_FILE;
    if(!s->ops.auto_user)return VMS_LOGIN_NOAUTO;
    char name[13];
    char *device=s->terminal;
    if(device[0]=='_' && device[1]=='_')device++;
    if(s->ops.system_privilege)(void)s->ops.system_privilege(s->context,1);
    int found=s->ops.auto_user(s->context,device,name,sizeof name);
    if(s->ops.system_privilege)(void)s->ops.system_privilege(s->context,0);
    if(found<=0)return VMS_LOGIN_NOAUTO;
    upcase(name);
    if(!copy(s->username,sizeof s->username,name))return VMS_LOGIN_AUTH;
    if(s->ops.purge_typeahead)(void)s->ops.purge_typeahead(s->context);
    int rc=select_user(s,s->username);
    if(rc!=VMS_LOGIN_OK)return rc;
    if(s->ops.set_username && s->ops.set_username(s->context,s->username)<0)
        return VMS_LOGIN_FILE;
    s->auto_selected=1;
    rc=vms_lgi_get_password(s);
    if(rc!=VMS_LOGIN_OK)return rc;
    s->logged_in=1;
    return VMS_LOGIN_OK;
}
int vms_lgi_interactive_validation(struct vms_lgi_interact *s){
    if(!s||!s->streams)return VMS_LOGIN_FILE;
    char text[128];
    for(unsigned attempts=0;attempts<4;attempts++){
        int rc=vms_lgi_get_input(s->streams,(uint8_t*)text,sizeof text);
        if(rc!=VMS_LGI_OK)return VMS_LOGIN_FILE;
        if(!text[0])continue;
        char full[136]="LOGIN ";
        unsigned prefix=6u,n=len(text);
        if(n>sizeof full-prefix-1u)return VMS_LOGIN_BADCLI;
        for(unsigned i=0;i<n;i++)full[prefix+i]=text[i];
        full[prefix+n]=0;
        rc=vms_login_parse(full,&s->command);
        if(rc!=VMS_LOGIN_CMD_OK)continue;
        if(!copy(s->username,sizeof s->username,s->command.username))
            return VMS_LOGIN_BADCLI;
        if(s->command.has_cli){
            char cli[80]="SYS$SYSTEM:";
            unsigned base=len(cli),sub=len(s->command.cli);
            if(base+sub>=sizeof cli)return VMS_LOGIN_BADCLI;
            for(unsigned i=0;i<sub;i++)cli[base+i]=s->command.cli[i];
            cli[base+sub]=0;
            if(!copy(s->command.cli,sizeof s->command.cli,cli))
                return VMS_LOGIN_BADCLI;
        }
        int lookup=select_user(s,s->username);
        rc=vms_lgi_get_password(s);
        if(lookup==VMS_LOGIN_FILE)return VMS_LOGIN_FILE;
        if(!s->user_present || rc!=VMS_LOGIN_OK)return VMS_LOGIN_AUTH;
        uint32_t bits=flags(s);
        if(bits&VMS_LOGIN_FLAG_DISABLED)return VMS_LOGIN_DISABLED;
        if((bits&VMS_LOGIN_FLAG_CAPTIVE) &&
          (s->command.has_cli || s->command.has_disk ||
           s->command.has_command_value || !s->command.command_enabled))
            return VMS_LOGIN_CAPTIVE;
        if(s->ops.set_username && s->ops.set_username(s->context,s->username)<0)
            return VMS_LOGIN_FILE;
        s->logged_in=1;
        return VMS_LOGIN_OK;
    }
    return VMS_LOGIN_BADCLI;
}
int vms_lgi_init_interactive(struct vms_lgi_interact *s){
    if(!s||!s->streams||!s->uaf)return VMS_LOGIN_FILE;
    if(vms_lgi_open_output(s->streams)!=VMS_LGI_OK ||
       vms_lgi_open_input(s->streams)!=VMS_LGI_OK)return VMS_LOGIN_FILE;
    if(s->ops.terminal_name &&
       s->ops.terminal_name(s->context,s->terminal,sizeof s->terminal)<0)
        return VMS_LOGIN_FILE;
    if(s->ops.set_termname &&
       s->ops.set_termname(s->context,s->terminal)<0)return VMS_LOGIN_FILE;
    if(s->ops.set_username &&
       s->ops.set_username(s->context,"<login>")<0)return VMS_LOGIN_FILE;
    int ann=vms_lgi_write_announcement(s,"SYS$ANNOUNCE");
    if(ann==VMS_LOGIN_NOAUTO){if(!output_blanks(s))return VMS_LOGIN_FILE;}
    else if(ann!=VMS_LOGIN_OK)return ann;
    int rc=vms_lgi_auto_login(s);
    if(rc==VMS_LOGIN_NOAUTO)rc=vms_lgi_interactive_validation(s);
    s->last_error=rc;
    return rc;
}
