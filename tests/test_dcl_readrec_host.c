#include <assert.h>
#include <string.h>
#include "vms_dcl_readrec.h"

struct test_ctx {
    const char *interactive[12];
    const char *procedure[12];
    unsigned interactive_at,procedure_at,retries,read_calls,canceled;
    unsigned pushed,popped,verified,errors,statuses,restarts,logouts,disabled,enabled;
    char last_verified[PORT_DCL_RECORD_SIZE];
};
static int read_line(void *context,unsigned depth,const char *prompt,
                     char *line,unsigned cap){
    struct test_ctx *c=context;
    (void)prompt;
    c->read_calls++;
    if(c->retries){c->retries--;return PORT_DCL_INPUT_RSA;}
    const char *s=depth?c->procedure[c->procedure_at++]:c->interactive[c->interactive_at++];
    if(!s)return PORT_DCL_INPUT_EOF;
    unsigned n=(unsigned)strlen(s);
    if(n+1u>cap)return PORT_DCL_INPUT_SPACE;
    memcpy(line,s,n+1u);
    return (int)n;
}
static int cancel(void *context,unsigned depth){
    struct test_ctx *c=context;
    (void)depth;c->canceled++;return 1;
}
static void disable_control_y(void *context){((struct test_ctx *)context)->disabled++;}
static void enable_control_y(void *context){((struct test_ctx *)context)->enabled++;}
static int push(void *context,unsigned depth,const char *spec){
    struct test_ctx *c=context;
    if(depth!=1u||strcmp(spec,"LAB.COM"))return 0;
    c->pushed++;c->procedure_at=0;return 1;
}
static void pop(void *context,unsigned depth){
    struct test_ctx *c=context;
    assert(depth==1u);c->popped++;
}
static int lookup(void *context,const char *sym,char *value,unsigned cap){
    (void)context;
    const char *v=0;
    if(!strcmp(sym,"WORLD"))v="Earth";
    if(!strcmp(sym,"NEST"))v="'WORLD'";
    if(!strcmp(sym,"LOOP"))v="'LOOP'";
    if(!strcmp(sym,"COMMAND"))v="SHOW";
    if(!v||strlen(v)+1u>cap)return 0;
    strcpy(value,v);return 1;
}
static int lex(void *context,const char *sym,char *value,unsigned cap){
    (void)context;
    if(strcmp(sym,"F$VERIFY(0)")&&strcmp(sym,"F$VERIFY"))return 0;
    if(cap<2u)return 0;
    strcpy(value,"1");return 1;
}
static void verify(void *context,const char *line){
    struct test_ctx *c=context;
    c->verified++;strcpy(c->last_verified,line);
}
static void error(void *context,int code){
    struct test_ctx *c=context;(void)code;c->errors++;
}
static void status(void *context,int code){
    struct test_ctx *c=context;(void)code;c->statuses++;
}
static void restart(void *context){((struct test_ctx *)context)->restarts++;}
static void logout(void *context){((struct test_ctx *)context)->logouts++;}
static const struct port_dcl_readrec_ops ops={
    .get=read_line,.cancel=cancel,.push=push,.pop=pop,
    .disable_control_y=disable_control_y,.enable_control_y=enable_control_y,
    .symbol=lookup,.lexical=lex,.verify=verify,.error=error,
    .set_status=status,.restart=restart,.logout=logout
};
int main(void){
    struct port_dcl_readrec p;
    struct test_ctx c={0};
    port_dcl_readrec_init(&p,0);
    c.interactive[0]=" ! comment";
    c.interactive[1]="SHOW 'WORLD'";
    p.verify=1;
    assert(port_dcl_input(&p,&ops,&c)==PORT_DCL_INPUT_OK);
    assert(!strcmp(p.line,"SHOW Earth")&&p.last_substitutions==1u);
    assert(c.interactive_at==2u&&c.verified==2u);
    assert(!strcmp(c.last_verified,"SHOW Earth"));
    strcpy(p.line,"'NEST'");
    assert(port_dcl_expand(&p,&ops,&c)==PORT_DCL_INPUT_OK);
    assert(!strcmp(p.line,"Earth")&&p.last_substitutions==2u);
    strcpy(p.line,"SAY \"'WORLD'\" \"''WORLD'\"");
    assert(port_dcl_expand(&p,&ops,&c)==PORT_DCL_INPUT_OK);
    assert(!strcmp(p.line,"SAY \"'WORLD'\" \"Earth\""));
    strcpy(p.line,"! 'MISSING' F$VERIFY(0) 'F$VERIFY(0)'");
    assert(port_dcl_expand(&p,&ops,&c)==PORT_DCL_INPUT_OK);
    assert(!strcmp(p.line,"! 'MISSING' F$VERIFY(0) 1"));
    strcpy(p.line,"'MISSING'");
    assert(port_dcl_expand(&p,&ops,&c)==PORT_DCL_INPUT_SYMBOL);
    strcpy(p.line,"'LOOP'");
    assert(port_dcl_expand(&p,&ops,&c)==PORT_DCL_INPUT_RECURSE);
    char replacement[PORT_DCL_RECORD_SIZE];
    assert(port_dcl_special(&p,"&COMMAND",&ops,&c,replacement,sizeof replacement)==PORT_DCL_INPUT_OK);
    assert(!strcmp(replacement,"SHOW"));
    assert(port_dcl_special(&p,"&ABSENT",&ops,&c,replacement,sizeof replacement)==PORT_DCL_INPUT_SYMBOL);
    assert(port_dcl_special(&p,"@LAB.COM",&ops,&c,replacement,sizeof replacement)==PORT_DCL_INPUT_INDIRECT);
    assert(p.depth==1u&&c.pushed==1u);
    c.procedure[0]="$ SHOW 'WORLD'";
    c.procedure[1]="! skip";
    c.interactive[2]="BACK FROM SCRIPT";
    assert(port_dcl_input(&p,&ops,&c)==PORT_DCL_INPUT_OK);
    assert(p.depth==1u&&!strcmp(p.line,"$ SHOW Earth"));
    assert(port_dcl_input(&p,&ops,&c)==PORT_DCL_INPUT_OK);
    assert(p.depth==0u&&!strcmp(p.line,"BACK FROM SCRIPT")&&c.popped==1u);
    p.goto_search=1;
    c.interactive[3]="SKIP";
    c.interactive[4]="TARGET: $ SHOW WORLD";
    assert(port_dcl_input(&p,&ops,&c)==PORT_DCL_INPUT_OK);
    assert(!strcmp(p.line,"TARGET: $ SHOW WORLD"));
    p.goto_search=0;
    c.retries=3u;
    c.interactive[5]="AFTER BUSY";
    assert(port_dcl_input(&p,&ops,&c)==PORT_DCL_INPUT_OK);
    assert(p.last_retry_count==3u&&!strcmp(p.line,"AFTER BUSY"));
    p.trailing_space=1;
    c.interactive[6]="CONTINUED";
    assert(port_dcl_input(&p,&ops,&c)==PORT_DCL_INPUT_OK);
    assert(!strcmp(p.line," CONTINUED"));
    c.interactive[7]="BAD 'MISSING'";
    assert(port_dcl_input(&p,&ops,&c)==PORT_DCL_INPUT_SYMBOL);
    assert(c.errors==1u&&c.statuses==1u&&c.restarts==1u&&!p.length);
    p.auto_logout=1;
    assert(port_dcl_input(&p,&ops,&c)==PORT_DCL_INPUT_LOGOUT);
    assert(c.logouts==1u);
    port_dcl_readrec_init(&p,1);
    struct test_ctx batch={0};
    assert(port_dcl_input(&p,&ops,&batch)==PORT_DCL_INPUT_LOGOUT);
    assert(batch.logouts==1u);
    assert(c.disabled==c.enabled&&c.disabled==c.read_calls);
    assert(port_dcl_special(&p,"@LAB.COM",&ops,&batch,
                            replacement,sizeof replacement)==PORT_DCL_INPUT_INDIRECT);
    p.goto_search=1;
    assert(port_dcl_special(&p,"@LAB.COM",&ops,&batch,
                            replacement,sizeof replacement)==PORT_DCL_INPUT_OK);
    assert(p.depth==1u);
    p.goto_search=0;
    p.indirect_enabled=0;
    assert(port_dcl_special(&p,"@LAB.COM",&ops,&batch,
                            replacement,sizeof replacement)==PORT_DCL_INPUT_OK);
    assert(p.depth==1u);
    p.indirect_enabled=1;
    p.depth=PORT_DCL_INDIRECT_LEVELS;
    assert(port_dcl_special(&p,"@LAB.COM",&ops,&batch,
                            replacement,sizeof replacement)==PORT_DCL_INPUT_BADFILE);
    return 0;
}
