#include <assert.h>
#include <string.h>
#include "vms_dcl_inquire.h"
struct c {int calls,global,disabled,control;char prompt[128],name[64],value[128];};
static int input(void *p,const char *s,char *answer,unsigned cap){
 struct c *c=p;assert(cap>16);strcpy(c->prompt,s);strcpy(answer,"\"Hello  world\"");c->calls++;return (int)strlen(answer);
}
static int store(void *p,const char *name,const char *v,unsigned g){
 struct c *c=p;strcpy(c->name,name);strcpy(c->value,v);c->global=(int)g;return 1;
}
static void dis(void *p){((struct c*)p)->disabled++;}
static void ctl(void *p){((struct c*)p)->control++;}
int main(void){
 struct port_dcl_inquire_request q;
 assert(port_dcl_inquire_parse("/GLOBAL/NOPUNCTUATION Answer \"Your name\"",&q)==1);
 assert(q.global==1 && q.punctuation==0 && !strcmp(q.symbol,"ANSWER") && !strcmp(q.prompt,"Your name"));
 assert(port_dcl_inquire_parse("/G foo",&q)==1 && q.global==1);
 assert(port_dcl_inquire_parse("foo",&q)==1 && !strcmp(q.prompt,"FOO: "));
 assert(port_dcl_inquire_parse("foo/NOPUNCTUATION",&q)==1 && !strcmp(q.prompt,"FOO"));
 assert(port_dcl_inquire_parse("/LOCAL/BOGUS foo",&q)==0);
 assert(port_dcl_inquire_parse("123 foo",&q)==0);
 assert(port_dcl_inquire_parse("foo \"unclosed",&q)==0);
 assert(port_dcl_inquire_parse("name \"A \"\"quote\"\" here\"",&q)==1 && !strcmp(q.prompt,"A \"quote\" here: "));
 const struct port_dcl_inquire_ops ops={input,store,dis,ctl};
 struct c c={0};
 assert(port_dcl_inquire("/GLOBAL name \"What name\"",&ops,&c)==1);
 assert(!strcmp(c.prompt,"What name: ") && !strcmp(c.name,"NAME") && !strcmp(c.value,"Hello  world") && c.global && c.disabled==1 && c.control==1);
 assert(port_dcl_inquire("/BAD name",&ops,&c)==0 && c.calls==1);
 return 0;
}
