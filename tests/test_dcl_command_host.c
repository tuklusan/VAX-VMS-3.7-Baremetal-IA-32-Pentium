#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "vms_dcl_command.h"

enum{ SHOW=0xA2,INQUIRE=0x91,LOGOUT=0x92,ON=0x94,SET=0x99 };
static const struct port_dcl_command_rule rules[]={
    {"SHOW",SHOW,PORT_DCL_CMD_INTERNAL,2,2,{0,0},{0}},
    {"SET",SET,PORT_DCL_CMD_INTERNAL,2,2,{0,0},{0}},
    {"INQUIRE",INQUIRE,PORT_DCL_CMD_INTERNAL,1,2,{0,0},{0}},
    {"ON",ON,PORT_DCL_CMD_INTERNAL,3,5,{0,0},{0}},
    {"LOGOUT",LOGOUT,PORT_DCL_CMD_INTERNAL,0,0,{0},{0}},
    {"LIST",0x89,PORT_DCL_CMD_INTERNAL,1,2,{PORT_DCL_CMD_LIST|PORT_DCL_CMD_CONCAT,0},{0}},
    {"ASK",0x88,PORT_DCL_CMD_INTERNAL,1,1,{0},{"Which one"}},
    {"FOREIGN",0,PORT_DCL_CMD_FOREIGN,0,1,{0},{0}}
};
struct test_context {
    char assigned_name[100], assigned_value[510];
    char lastverb[510];
    unsigned global,numeric,substring,dispatches,labels,opens,flushes,disables,enables;
    unsigned status,errors,starts,external,reads;
    int last_error;
    char alias[510];
};
static const struct port_dcl_command_rule *find(void *ctx,const char *v){
    (void)ctx;
    for(unsigned i=0;i<sizeof rules/sizeof rules[0];i++)
        if(!strcmp(v,rules[i].name))return &rules[i];
    return 0;
}
static int symbol(void *ctx,const char *v,char *out,unsigned cap){
    struct test_context *c=ctx;
    if(strcmp(v,"ALIAS"))return 0;
    if(strlen(c->alias)>=cap)return 0;
    strcpy(out,c->alias);return 1;
}
static int assign(void *ctx,const char *n,const char *v,unsigned g,unsigned nu,unsigned sub){
    struct test_context *c=ctx;
    strcpy(c->assigned_name,n);
    strcpy(c->assigned_value,v);
    c->global=g;c->numeric=nu;c->substring=sub;
    return 1;
}
static int label(void *ctx,const char *n){
    struct test_context *c=ctx;c->labels++;
    strcpy(c->lastverb,n);return 1;
}
static int seek(void *ctx,const char *n){
    return !strcmp(n,"FOUND") && (++((struct test_context*)ctx)->starts!=0);
}
static int opn(void *ctx,const char *name){
    struct test_context *c=ctx;c->opens++;
    return !strcmp(name,"LAB.COM");
}
static int read_param(void *ctx,const char *question,char *out,unsigned cap){
    struct test_context *c=ctx;
    if(strcmp(question,"Which one")||cap<6)return -1;
    strcpy(out,"input");c->reads++;return 5;
}
static int dispatch(void *ctx,const struct port_dcl_command *s,
                    const struct port_dcl_command_rule *rule){
    struct test_context *c=ctx;
    c->dispatches++;
    strcpy(c->lastverb,s->verb);
    c->external=(rule->flags&PORT_DCL_CMD_FOREIGN)!=0;
    if(!strcmp(s->verb,"LOGOUT"))return PORT_DCL_CMD_LOGOUT;
    return PORT_DCL_CMD_OK;
}
static void flush(void *ctx){((struct test_context*)ctx)->flushes++;}
static void ast(void *ctx,unsigned e){
    struct test_context *c=ctx;
    if(e)c->enables++;else c->disables++;
}
static int force(void *ctx){((struct test_context*)ctx)->starts++;return 1;}
static void report(void *ctx,int code){
    struct test_context *c=ctx;c->errors++;c->last_error=code;
}
static void status(void *ctx,uint32_t value){((struct test_context*)ctx)->status=value;}
static const struct port_dcl_command_ops ops={
    .find_verb=find,.symbol=symbol,.assign=assign,.label=label,
    .seek_label=seek,.open_procedure=opn,.read_parameter=read_param,
    .dispatch=dispatch,.flush=flush,.control_ast=ast,.force_exit=force,
    .report=report,.set_status=status
};
static struct port_dcl_command s;
static struct test_context c;
#define RUN(X,EXPECT) assert(port_dcl_command_run(&s,X,&ops,&c)==(EXPECT))
int main(void){
    port_dcl_command_init(&s);
    assert(!strcmp(s.prompt,"$ ")&&s.last_status==1);
    RUN("  $ SHOW SYMBOL DEST",PORT_DCL_CMD_OK);
    assert(s.image==SHOW&&s.param_count==2&&s.qual_count==0);
    assert(!strcmp(port_dcl_cmd_parameter(&s,0)->value,"SYMBOL"));
    assert(!strcmp(port_dcl_cmd_parameter(&s,1)->value,"DEST"));
    RUN("INQUIRE/GLOBAL DEST \"Which \"\"place\"\"\"",PORT_DCL_CMD_OK);
    assert(s.qual_count==1&&!strcmp(port_dcl_cmd_option(&s,"global")->value,"GLOBAL"));
    assert(!strcmp(port_dcl_cmd_parameter(&s,0)->value,"DEST"));
    assert(!strcmp(port_dcl_cmd_parameter(&s,1)->value,"Which \"place\""));
    RUN("ON ERROR THEN EXIT",PORT_DCL_CMD_OK);
    assert(s.param_count==3&&s.image==ON);
    RUN("SET HOST 0",PORT_DCL_CMD_OK);
    assert(s.param_count==2&&!strcmp(port_dcl_cmd_parameter(&s,1)->value,"0"));
    RUN("ALPHA := \"Alpha Beta\"",PORT_DCL_CMD_OK);
    assert(!strcmp(c.assigned_name,"ALPHA")&&!strcmp(c.assigned_value,"\"Alpha Beta\"")&&
           !c.global&&!c.numeric&&!c.substring);
    RUN("GLOBAL :== Mars",PORT_DCL_CMD_OK);
    assert(!strcmp(c.assigned_name,"GLOBAL")&&c.global&&!c.numeric);
    RUN("NUM = 4+3",PORT_DCL_CMD_OK);
    assert(!strcmp(c.assigned_name,"NUM")&&c.numeric&&!c.global);
    RUN("NUM == 7",PORT_DCL_CMD_OK);
    assert(c.global&&c.numeric);
    RUN("C[0,2] = 5",PORT_DCL_CMD_OK);
    assert(!strcmp(c.assigned_name,"C[0,2]")&&c.substring);
    s.depth=1;
    RUN("LABEL:",PORT_DCL_CMD_SKIPPED);
    assert(c.labels==1&&!strcmp(c.lastverb,"LABEL"));
    s.goto_pending=1;
    RUN("OTHER:",PORT_DCL_CMD_SKIPPED);
    assert(s.goto_pending==1);
    RUN("FOUND:",PORT_DCL_CMD_SKIPPED);
    assert(!s.goto_pending);
    s.depth=0;
    RUN("LABEL:",PORT_DCL_CMD_SKIPPED);
    assert(c.labels==1);
    RUN("@LAB.COM",PORT_DCL_CMD_SKIPPED);
    assert(c.opens==1);
    RUN("@NOFILE.COM",PORT_DCL_CMD_INPUT);
    strcpy(c.alias,"SHOW");
    RUN("ALIAS SYMBOL DEST",PORT_DCL_CMD_OK);
    assert(!strcmp(s.verb,"SHOW")&&s.param_count==2);
    strcpy(c.alias,"$SYS$SYSTEM:FOO");
    RUN("ALIAS /LIST file",PORT_DCL_CMD_OK);
    assert(c.external&&!strcmp(s.external_image,"SYS$SYSTEM:FOO")&&s.param_count==1);
    strcpy(c.alias,"! a comment");
    RUN("ALIAS anything",PORT_DCL_CMD_SKIPPED);
    strcpy(c.alias,"ALIAS");
    RUN("ALIAS",PORT_DCL_CMD_RECURSE);
    RUN("LIST 1,2+3 TEST",PORT_DCL_CMD_OK);
    assert(s.param_count==2&&s.count>=4&&s.tokens[0].separator==','&&
           s.tokens[1].separator=='+');
    RUN("LIST 1+2",PORT_DCL_CMD_OK);
    RUN("INQUIRE/GLOBAL/OPTIONS=(A,B) DEST """,PORT_DCL_CMD_OK);
    assert(s.qual_count==2&&
           !strcmp(s.tokens[3].value,"(A,B)"));
    RUN("SHOW SYMBOL,DEST",PORT_DCL_CMD_NOLIST);
    RUN("SHOW SYMBOL+DEST",PORT_DCL_CMD_NOCONCAT);
    RUN("SHOW SYMBOL DEST EXTRA",PORT_DCL_CMD_MAXPARM);
    RUN("UNKNOWN X",PORT_DCL_CMD_BADVERB);
    assert(c.errors>=4&&c.status==2);
    RUN("ASK",PORT_DCL_CMD_OK);
    assert(c.reads==1&&!strcmp(port_dcl_cmd_parameter(&s,0)->value,"input"));
    s.batch=1;
    RUN("ASK",PORT_DCL_CMD_MISSING);
    s.batch=0;
    assert(port_dcl_cmd_eod(&s,"   ")==PORT_DCL_CMD_OK&&s.no_status);
    assert(port_dcl_cmd_eod(&s,"INVALID")==PORT_DCL_CMD_SYNTAX);
    assert(port_dcl_cmd_check_ast(&s)==PORT_DCL_CMD_OK);
    s.control_y_pending=1;
    assert(port_dcl_cmd_check_ast(&s)==PORT_DCL_CMD_RESTART);
    s.control_y_pending=0;
    port_dcl_cmd_disable(&s,&ops,&c);
    port_dcl_cmd_disable(&s,&ops,&c);
    assert(c.disables==1);
    port_dcl_cmd_enable(&s,&ops,&c);
    assert(!c.enables);
    port_dcl_cmd_enable(&s,&ops,&c);
    assert(c.enables==1);
    port_dcl_cmd_flush(&s,0,&ops,&c);
    assert(c.flushes==1&&s.flushes==1);
    port_dcl_cmd_flush(&s,1,&ops,&c);
    assert(c.flushes==1);
    s.privileged_image=1;
    RUN("SHOW SYMBOL DEST",PORT_DCL_CMD_OK);
    assert(c.starts>=1&&!s.privileged_image);
    RUN("LOGOUT",PORT_DCL_CMD_LOGOUT);
    s.hangup=1;
    RUN("SHOW SYMBOL DEST",PORT_DCL_CMD_LOGOUT);
    puts("DCL COMMAND host state, parsing and service checks PASS");
    return 0;
}
