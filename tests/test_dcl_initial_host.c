#include <assert.h>
#include <stdint.h>
#include <string.h>
#include "vms_dcl_initial.h"
static struct port_dcl_initial p;
struct context {
    unsigned procs,logicals,reset,installed,rundown,restart,fatal,closed,deleted;
    unsigned new_priv,mailbox_calls,reads;
    uint32_t mask;
    char first_proc[24],second_proc[24];
};
static int translation(void *v,const char *name,char *out,unsigned cap){
    (void)v;(void)cap;
    if(!strcmp(name,"P1")){out[0]=(char)255;return 1;}
    if(!strcmp(name,"P2")){memcpy(out,"Earth",5);return 5;}
    if(!strcmp(name,"PROC2")){memcpy(out,"SYS$LOGIN.COM",13);return 13;}
    if(!strcmp(name,"PROC1")){memcpy(out,"SYLOGIN.COM",11);return 11;}
    return -1;
}
static int push(void *v,const char *file){
    struct context *c=v;
    if(!c->procs)strcpy(c->first_proc,file);
    else strcpy(c->second_proc,file);
    c->procs++;return 1;
}
static int reset(void *v,uint32_t mask){
    struct context *c=v;c->reset++;c->mask=mask;return 1;
}
static int install(void *v){((struct context*)v)->installed++;return 1;}
static void rundown(void *v){((struct context*)v)->rundown++;}
static void restart(void *v){((struct context*)v)->restart++;}
static void fatal(void *v,int code){(void)code;((struct context*)v)->fatal++;}
static int open_mailbox(void *v,char *device,unsigned cap,uint8_t *mode){
    (void)cap;((struct context*)v)->mailbox_calls++;
    strcpy(device,"_MBA0001");*mode=2u;return 1;
}
static void close_mailbox(void *v){((struct context*)v)->closed++;}
static void delete_error(void *v){((struct context*)v)->deleted++;}
static int privileges(void *v,const uint32_t priv[2]){
    struct context *c=v;
    if(priv[0]!=0x12345678u||priv[1]!=0x87654321u)return 0;
    c->new_priv++;return 1;
}
static int logical(void *v,uint8_t acmode,const char *name,const char *value){
    struct context *c=v;
    if(acmode!=2u||strcmp(name,"TEST")||strcmp(value,"LOGICAL"))return 0;
    c->logicals++;return 1;
}
static const struct port_dcl_initial_ops ops={
    .translate_process=translation,.push_procedure=push,
    .reset_oob=reset,.install_change_mode=install,
    .rundown_login=rundown,.restart=restart,.fatal=fatal,
    .open_spawn_mailbox=open_mailbox,.close_spawn_mailbox=close_mailbox,
    .delete_super_error=delete_error,
    .set_privileges=privileges,.create_logical=logical
};
static const struct port_dcl_initial_ppd template={
    .batch=1u,.input_isi=7u,.output_isi=7u,
    .input_ifi=11u,.output_ifi=12u,.input_devchar=0x1234u,
    .output_devchar=0xabcd,.input_device="_TTA0:",
    .output_device="_TTA0:",.initial_procedures=2u,
    .table_structure_level=5u
};
int main(void){
    struct context c={0};
    struct port_dcl_initial_ppd ppd=template;
    assert(port_dcl_initial_start(&p,&ppd,&ops,&c)==PORT_DCL_INITIAL_OK);
    assert(p.flags==0xc0u&&p.on_level==2u&&p.exam_width==4u);
    assert(p.depth==2u&&p.frame_capacity==9u&&p.frame_size==112u);
    assert(p.frames[0].content[4]==11u&&p.frames[0].content[6]==12u);
    assert(p.input.isi==7u&&p.output.isi==7u&&p.input.mbc==1u);
    assert(p.active_output==&p.input&&p.error_stream==p.active_output&&
           p.indirect_output==p.active_output);
    assert(p.input.prompt==1u&&p.alternate_input.prompt==1u);
    assert(c.procs==2u&&!strcmp(c.first_proc,"SYS$LOGIN.COM")&&
           !strcmp(c.second_proc,"SYLOGIN.COM"));
    assert(c.reset==1u&&c.installed==1u&&c.rundown==1u&&c.restart==1u);
    assert(c.mask==PORT_DCL_CTRLY_MASK&&p.control_y_enabled==1u);
    char str[300];
    assert(port_dcl_initial_symbol(&p,"P1",0,str,sizeof str)&&!strcmp(str,""));
    assert(port_dcl_initial_symbol(&p,"P2",0,str,sizeof str)&&!strcmp(str,"Earth"));
    assert(port_dcl_initial_symbol(&p,"$STATUS",1,str,sizeof str)&&!strcmp(str,"1"));
    port_dcl_initial_set_status(&p,0xdeadbeefu);
    assert(port_dcl_initial_symbol(&p,"$STATUS",1,str,sizeof str)&&
           !strcmp(str,"3735928559"));
    assert(port_dcl_initial_symbol(&p,"$SEVERITY",1,str,sizeof str)&&
           !strcmp(str,"7"));
    assert(port_dcl_get_prc(&p)==&p);
    ppd.batch=0u;ppd.no_control_y=1u;ppd.initial_procedures=0u;
    ppd.output_isi=9u;
    ppd.table_structure_level=3u;
    c=(struct context){0};
    assert(port_dcl_initial_start(&p,&ppd,&ops,&c)==PORT_DCL_INITIAL_OK);
    assert(p.on_level==0x208u&&p.verify==0u&&p.outofband_mask==0u);
    assert(p.active_output==&p.output&&p.error_stream==&p.output);
    assert(c.reset==1u&&c.mask==0u&&p.control_y_enabled==0u);
    assert(!port_dcl_initial_symbol(&p,"P2",0,str,sizeof str));
    ppd.table_structure_level=1u;
    assert(port_dcl_initial_start(&p,&ppd,&ops,&c)==PORT_DCL_INITIAL_BADSTR);
    assert(c.fatal==1u);
    ppd.table_structure_level=5u;ppd.parent_pid=76u;
    assert(port_dcl_initial_start(&p,&ppd,&ops,&c)==PORT_DCL_INITIAL_OK);
    uint8_t header[15]={0};
    header[2]=0x78u;header[3]=0x56u;header[4]=0x34u;header[5]=0x12u;
    header[6]=0x21u;header[7]=0x43u;header[8]=0x65u;header[9]=0x87u;
    header[10]=0xffu;header[11]=0xffu;header[12]=0xffu;header[13]=0xffu;
    header[14]=PORT_DCL_CTX_AUTOLOGO|PORT_DCL_CTX_EOFLOGO|PORT_DCL_CTX_VERIFY;
    assert(port_dcl_initial_spawn_record(&p,header,15u,9u,&ops,&c)==PORT_DCL_INITIAL_BADCTX);
    assert(port_dcl_initial_spawn_record(&p,header,15u,76u,&ops,&c)==PORT_DCL_INITIAL_OK);
    assert(c.new_priv==1u&&p.auto_logout&&p.eof_logout&&p.verify);
    uint8_t cmd[]={1u,0u,'S','H','O','W',' ','C','L','O','C','K'};
    assert(port_dcl_initial_spawn_record(&p,cmd,sizeof cmd,76u,&ops,&c)==PORT_DCL_INITIAL_OK);
    assert(p.command_pending&&port_dcl_initial_take_command(&p,str,sizeof str)&&
           !strcmp(str,"SHOW CLOCK")&&!p.command_pending);
    uint8_t log[]={2u,0u,0u,0u,2u,4u,'T','E','S','T',7u,0u,
                    'L','O','G','I','C','A','L'};
    assert(port_dcl_initial_spawn_record(&p,log,sizeof log,76u,&ops,&c)==PORT_DCL_INITIAL_OK);
    assert(c.logicals==1u);
    uint8_t sym[]={3u,0u,0u,0u,0u,0u,2u,3u,'C','I','T',4u,0u,
                   'M','a','r','s'};
    assert(port_dcl_initial_spawn_record(&p,sym,sizeof sym,76u,&ops,&c)==PORT_DCL_INITIAL_OK);
    assert(port_dcl_initial_symbol(&p,"CIT",1,str,sizeof str)&&!strcmp(str,"Mars"));
    uint8_t bin[]={3u,0u,0u,0u,1u,2u,0u,3u,'B','I','N',0x12,0x34,0x56,0x78};
    assert(port_dcl_initial_spawn_record(&p,bin,sizeof bin,76u,&ops,&c)==PORT_DCL_INITIAL_OK);
    assert(port_dcl_initial_symbol(&p,"BIN",0,str,sizeof str)&&
           !strcmp(str,"2018915346"));
    assert(port_dcl_initial_spawn_record(&p,bin,5u,76u,&ops,&c)==PORT_DCL_INITIAL_BADCTX);
    return 0;
}
