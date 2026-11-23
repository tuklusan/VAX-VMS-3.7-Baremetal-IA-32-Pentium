#include "vms_dcl_initial.h"

static void zero(void *data,unsigned count){
    uint8_t *p=data;
    for(unsigned i=0;i<count;i++)p[i]=0;
}
static unsigned size(const char *s){unsigned n=0;while(s&&s[n])n++;return n;}
static unsigned upper(unsigned c){return c>='a'&&c<='z'?c-32u:c;}
static int name_eq(const char *a,const char *b){
    while(*a&&*b){if(upper((uint8_t)*a++)!=upper((uint8_t)*b++))return 0;}
    return *a==*b;
}
static int copy(char *out,unsigned cap,const char *src,unsigned length){
    if(!out||!src||length>=cap)return 0;
    for(unsigned i=0;i<length;i++)out[i]=src[i];
    out[length]=0;
    return 1;
}
static uint32_t word32(const uint8_t *b){
    return (uint32_t)b[0]|((uint32_t)b[1]<<8)|
        ((uint32_t)b[2]<<16)|((uint32_t)b[3]<<24);
}
static unsigned word16(const uint8_t *b){return (unsigned)b[0]|((unsigned)b[1]<<8);}
static void status_text(struct port_dcl_initial *p,uint32_t status){
    char reverse[10];unsigned i=0;
    p->last_status=status;
    p->severity[0]=(char)('0'+((status>>1)&7u));p->severity[1]=0;
    do{reverse[i++]=(char)('0'+(status%10u));status/=10u;}while(status&&i<10u);
    for(unsigned j=0;j<i;j++)p->status[j]=reverse[i-1u-j];
    p->status[i]=0;
}
void port_dcl_initial_set_status(struct port_dcl_initial *p,uint32_t status){
    if(p)status_text(p,status);
}
static int store(struct port_dcl_initial *p,const char *name,unsigned n,
                 const char *value,unsigned length,uint32_t binary,
                 uint8_t global,uint8_t type,uint8_t nonunique){
    if(!n||n>=sizeof p->symbols[0].name||length>PORT_DCL_INITIAL_MAX_VALUE||
       (type!=PORT_DCL_CTX_STRING&&type!=PORT_DCL_CTX_PERM&&
        type!=PORT_DCL_CTX_BINARY))return PORT_DCL_INITIAL_BADCTX;
    for(unsigned i=0;i<p->symbols_used;i++){
        struct port_dcl_initial_sym *s=&p->symbols[i];
        if(s->global==global&&size(s->name)==n){
            int same=1;
            for(unsigned j=0;j<n;j++)if(upper((uint8_t)s->name[j])!=
                                            upper((uint8_t)name[j]))same=0;
            if(same){
                if(s->type==PORT_DCL_CTX_PERM)return PORT_DCL_INITIAL_BADCTX;
                if(!copy(s->value,sizeof s->value,value,length))return PORT_DCL_INITIAL_BADCTX;
                s->length=(uint16_t)length;s->binary=binary;
                s->type=type;s->nonunique=nonunique;return PORT_DCL_INITIAL_OK;
            }
        }
    }
    if(p->symbols_used>=PORT_DCL_INITIAL_MAX_SYMS)return PORT_DCL_INITIAL_MEMORY;
    struct port_dcl_initial_sym *s=&p->symbols[p->symbols_used++];
    if(!copy(s->name,sizeof s->name,name,n)||
       !copy(s->value,sizeof s->value,value,length))return PORT_DCL_INITIAL_BADCTX;
    s->length=(uint16_t)length;s->binary=binary;s->global=global;
    s->type=type;s->nonunique=nonunique;
    return PORT_DCL_INITIAL_OK;
}
int port_dcl_initial_symbol(const struct port_dcl_initial *p,const char *name,
                            unsigned global,char *out,unsigned cap){
    if(!p||!name||!out||!cap)return 0;
    if(name_eq(name,"$STATUS"))return copy(out,cap,p->status,size(p->status));
    if(name_eq(name,"$SEVERITY"))return copy(out,cap,p->severity,1u);
    for(unsigned i=p->symbols_used;i>0;i--){
        const struct port_dcl_initial_sym *s=&p->symbols[i-1u];
        if((global>1u||s->global==global)&&name_eq(s->name,name)){
            if(s->type!=PORT_DCL_CTX_BINARY)return copy(out,cap,s->value,s->length);
            char digits[11];unsigned n=0;uint32_t v=s->binary;
            do{digits[n++]=(char)('0'+v%10u);v/=10u;}while(v);
            if(n>=cap)return 0;
            for(unsigned j=0;j<n;j++)out[j]=digits[n-j-1u];
            out[n]=0;return 1;
        }
    }
    return 0;
}
const struct port_dcl_initial *port_dcl_get_prc(const struct port_dcl_initial *p){
    return p;
}
int port_dcl_initial_take_command(struct port_dcl_initial *p,char *out,unsigned cap){
    if(!p||!out||!p->command_pending)return 0;
    if(!copy(out,cap,p->pending_command,p->command_length))return 0;
    p->command_pending=0;p->command_length=0;p->pending_command[0]=0;
    return 1;
}
/* CTX record fields and the sender check match the source's 256-byte
   mailbox records. The caller supplies sender PID from a real mailbox. */
int port_dcl_initial_spawn_record(struct port_dcl_initial *p,
                                  const uint8_t *b,unsigned length,
                                  uint32_t sender,
                                  const struct port_dcl_initial_ops *ops,void *ctx){
    if(!p||!b||length<2u||length>PORT_DCL_INITIAL_CTX_MAX||
       !p->ppd.parent_pid||sender!=p->ppd.parent_pid)
        return PORT_DCL_INITIAL_BADCTX;
    const unsigned type=word16(b);
    if(type==PORT_DCL_CTX_HEADER){
        if(length<15u||!ops||!ops->set_privileges)return PORT_DCL_INITIAL_BADCTX;
        const uint32_t priv[2]={word32(b+2u),word32(b+6u)};
        if(!ops->set_privileges(ctx,priv))return PORT_DCL_INITIAL_SERVICE;
        p->privileges[0]=priv[0];p->privileges[1]=priv[1];
        p->auto_logout=(b[14]&PORT_DCL_CTX_AUTOLOGO)!=0;
        p->eof_logout=(b[14]&PORT_DCL_CTX_EOFLOGO)!=0;
        p->verify=(b[14]&PORT_DCL_CTX_VERIFY)!=0;
        p->outofband_mask=word32(b+10u);
        return PORT_DCL_INITIAL_OK;
    }
    if(type==PORT_DCL_CTX_CMDSTR){
        if(length-2u>=sizeof p->pending_command)return PORT_DCL_INITIAL_BADCTX;
        if(!copy(p->pending_command,sizeof p->pending_command,
                 (const char *)b+2u,length-2u))return PORT_DCL_INITIAL_BADCTX;
        p->command_length=length-2u;p->command_pending=1;
        return PORT_DCL_INITIAL_OK;
    }
    if(type==PORT_DCL_CTX_LOGNAM){
        if(length<8u||!ops||!ops->create_logical)return PORT_DCL_INITIAL_BADCTX;
        uint8_t mode=b[4];
        if(mode<2u)return PORT_DCL_INITIAL_OK;
        unsigned count=b[5];
        if(count==0||6u+count+2u>length||count>=64u)return PORT_DCL_INITIAL_BADCTX;
        unsigned val_len=word16(b+6u+count);
        if(8u+count+val_len>length||val_len>PORT_DCL_INITIAL_MAX_VALUE)
            return PORT_DCL_INITIAL_BADCTX;
        char name[64],value[PORT_DCL_INITIAL_MAX_VALUE+1];
        if(!copy(name,sizeof name,(const char *)b+6u,count)||
           !copy(value,sizeof value,(const char *)b+8u+count,val_len))
            return PORT_DCL_INITIAL_BADCTX;
        return ops->create_logical(ctx,mode,name,value)?
               PORT_DCL_INITIAL_OK:PORT_DCL_INITIAL_SERVICE;
    }
    if(type==PORT_DCL_CTX_CLISYM){
        if(length<9u)return PORT_DCL_INITIAL_BADCTX;
        uint8_t table=b[4],sym_type=b[5],nonunique=b[6];
        unsigned name_len=b[7];
        if(table>PORT_DCL_CTX_LOCAL||sym_type>PORT_DCL_CTX_BINARY||
           !name_len||8u+name_len>length||name_len>=64u)
            return PORT_DCL_INITIAL_BADCTX;
        unsigned pos=8u+name_len;
        char name[64],value[PORT_DCL_INITIAL_MAX_VALUE+1];
        if(!copy(name,sizeof name,(const char *)b+8u,name_len))
            return PORT_DCL_INITIAL_BADCTX;
        if(sym_type==PORT_DCL_CTX_BINARY){
            if(pos+4u>length)return PORT_DCL_INITIAL_BADCTX;
            return store(p,name,name_len,"",0,word32(b+pos),
                         (uint8_t)(table==PORT_DCL_CTX_GLOBAL),sym_type,nonunique);
        }
        if(pos+2u>length)return PORT_DCL_INITIAL_BADCTX;
        unsigned val_len=word16(b+pos);pos+=2u;
        if(val_len>PORT_DCL_INITIAL_MAX_VALUE||pos+val_len>length||
           !copy(value,sizeof value,(const char *)b+pos,val_len))
            return PORT_DCL_INITIAL_BADCTX;
        return store(p,name,name_len,value,val_len,0,
                     (uint8_t)(table==PORT_DCL_CTX_GLOBAL),sym_type,nonunique);
    }
    return PORT_DCL_INITIAL_OK;
}
static int startup_fail(struct port_dcl_initial *p,int code,
                        const struct port_dcl_initial_ops *ops,void *ctx){
    if(ops&&ops->fatal)ops->fatal(ctx,code);
    p->last_status=(uint32_t)code;
    return code;
}
int port_dcl_initial_start(struct port_dcl_initial *p,
                           const struct port_dcl_initial_ppd *ppd,
                           const struct port_dcl_initial_ops *ops,void *ctx){
    if(!p||!ppd)return PORT_DCL_INITIAL_BADCTX;
    zero(p,sizeof *p);
    p->ppd=*ppd;
    p->table_level=ppd->table_structure_level;
    p->on_level=(uint16_t)(ppd->batch?2u:((2u<<8)|8u));
    p->flags=(uint16_t)(ppd->batch?0x00c0u:0u);
    p->verify=ppd->batch;
    p->exam_width=4u;p->exam_mode=0u;
    p->outofband_mask=ppd->no_control_y?0u:PORT_DCL_CTRLY_MASK;
    p->frame_size=PORT_DCL_INITIAL_IDF_BYTES;
    p->frame_capacity=PORT_DCL_INITIAL_FRAMES;
    p->frame_stack_offset=8u*PORT_DCL_INITIAL_IDF_BYTES;
    p->input.length=p->output.length=PORT_DCL_INITIAL_RAB_LEN;
    p->alternate_input.length=p->alternate_output.length=PORT_DCL_INITIAL_RAB_LEN;
    p->fab_length=PORT_DCL_INITIAL_FAB_LEN;
    p->nam_length=PORT_DCL_INITIAL_NAM_LEN;
    p->input.isi=ppd->input_isi;p->output.isi=ppd->output_isi;
    p->alternate_input.isi=ppd->input_isi;p->alternate_output.isi=ppd->output_isi;
    p->input.ifi=ppd->input_ifi;p->output.ifi=ppd->output_ifi;
    p->input.device_characteristics=ppd->input_devchar;
    p->output.device_characteristics=ppd->output_devchar;
    p->alternate_input.device_characteristics=ppd->input_devchar;
    p->alternate_output.device_characteristics=ppd->output_devchar;
    p->input.mbc=p->output.mbc=p->alternate_input.mbc=p->alternate_output.mbc=1u;
    p->input.mbf=p->output.mbf=p->alternate_input.mbf=p->alternate_output.mbf=255u;
    p->input.prompt=p->alternate_input.prompt=1u;
    p->active_input=&p->input;
    p->active_output=ppd->input_isi==ppd->output_isi?&p->input:&p->output;
    p->error_stream=p->active_output;
    p->indirect_input=p->active_input;
    p->indirect_output=p->active_output;
    /* IDF level zero: offsets follow SHRLIB/DCLDEF.B32. */
    uint8_t *frame=p->frames[0].content;
    frame[4]=(uint8_t)ppd->input_ifi;frame[5]=(uint8_t)(ppd->input_ifi>>8);
    frame[6]=(uint8_t)ppd->output_ifi;frame[7]=(uint8_t)(ppd->output_ifi>>8);
    frame[8]=(uint8_t)ppd->output_isi;frame[9]=(uint8_t)(ppd->output_isi>>8);
    const uint32_t in=ppd->input_devchar,out=ppd->output_devchar;
    for(unsigned j=0;j<4;j++){
        frame[16+j]=(uint8_t)(in>>(j*8u));
        frame[20+j]=(uint8_t)(out>>(j*8u));
    }
    for(unsigned j=0;j<16;j++){
        frame[40+j]=(uint8_t)ppd->output_device[j];
        frame[56+j]=(uint8_t)ppd->input_device[j];
    }
    status_text(p,1u);
    if(store(p,"$STATUS",7,p->status,size(p->status),0,1,
             PORT_DCL_CTX_PERM,0)!=PORT_DCL_INITIAL_OK||
       store(p,"$SEVERITY",9,p->severity,1u,0,1,
             PORT_DCL_CTX_PERM,0)!=PORT_DCL_INITIAL_OK)
        return startup_fail(p,PORT_DCL_INITIAL_MEMORY,ops,ctx);
    if(ppd->batch){
        for(unsigned j=1;j<=8;j++){
            char name[3]={'P',(char)('0'+j),0};
            char value[PORT_DCL_INITIAL_MAX_VALUE+1];int n=-1;
            if(ops&&ops->translate_process)n=ops->translate_process(ctx,name,value,sizeof value);
            if(n<0)n=0;
            if(n>64)
                return startup_fail(p,PORT_DCL_INITIAL_BADCTX,ops,ctx);
            if(n==1&&(uint8_t)value[0]==255u)n=0;
            value[n]=0;
            if(store(p,name,2u,value,(unsigned)n,0,0,
                     PORT_DCL_CTX_STRING,0)!=PORT_DCL_INITIAL_OK)
                return startup_fail(p,PORT_DCL_INITIAL_MEMORY,ops,ctx);
        }
    }
    /* Initial PROC# logical names are scanned from high to low. */
    for(unsigned j=ppd->initial_procedures;j>0;j--){
        char name[8]={'P','R','O','C','0',0};
        if(j>9u){name[4]=(char)('0'+(j/10u));name[5]=(char)('0'+j%10u);name[6]=0;}
        else name[4]=(char)('0'+j);
        char value[PORT_DCL_INITIAL_MAX_VALUE+1];
        int n=ops&&ops->translate_process?
            ops->translate_process(ctx,name,value,sizeof value):-1;
        if(n>0&&n<=(int)PORT_DCL_INITIAL_MAX_VALUE){
            value[n]=0;
            if(ops&&ops->push_procedure&&ops->push_procedure(ctx,value)){
                if(p->depth>=8u)return startup_fail(p,PORT_DCL_INITIAL_MEMORY,ops,ctx);
                p->depth++;
            }
        }
    }
    if(ops&&ops->rundown_login)ops->rundown_login(ctx);
    if(ppd->parent_pid&&ops&&ops->open_spawn_mailbox&&ops->read_spawn_record){
        char dev[64];uint8_t mode=0;
        if(ops->open_spawn_mailbox(ctx,dev,sizeof dev,&mode)&&
           mode==2u&&dev[0]=='_'&&dev[1]=='M'&&dev[2]=='B'&&dev[3]=='A'){
            uint8_t record[PORT_DCL_INITIAL_CTX_MAX];uint32_t sender;
            int bytes;
            while((bytes=ops->read_spawn_record(ctx,record,sizeof record,&sender))>0){
                if((unsigned)bytes>sizeof record)break;
                if(sender!=ppd->parent_pid)break;
                (void)port_dcl_initial_spawn_record(p,record,(unsigned)bytes,
                                                     sender,ops,ctx);
            }
            if(ops->close_spawn_mailbox)ops->close_spawn_mailbox(ctx);
            if(ops->delete_super_error)ops->delete_super_error(ctx);
        }else if(ops->close_spawn_mailbox)ops->close_spawn_mailbox(ctx);
    }
    if(p->table_level<2u||p->table_level>5u)
        return startup_fail(p,PORT_DCL_INITIAL_BADSTR,ops,ctx);
    p->control_y_enabled=(p->outofband_mask&PORT_DCL_CTRLY_MASK)!=0u;
    if(ops&&ops->reset_oob&&
       !ops->reset_oob(ctx,p->outofband_mask))
        return startup_fail(p,PORT_DCL_INITIAL_SERVICE,ops,ctx);
    if(ops&&ops->install_change_mode&&
       !ops->install_change_mode(ctx))
        return startup_fail(p,PORT_DCL_INITIAL_SERVICE,ops,ctx);
    if(ops&&ops->restart)ops->restart(ctx);
    return PORT_DCL_INITIAL_OK;
}
