#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "vms_boo_readprompt.h"
struct test {unsigned a,r,w;uint16_t channel;char last[32];};
static uint32_t assign(void *v,const char *name,uint16_t *channel){struct test *t=v;assert(!strcmp(name,"_OPA0"));t->a++;*channel=14;return 1;}
static uint32_t readp(void *v,uint16_t ch,const char *s,uint16_t sl,uint8_t *buf,uint16_t cap,uint16_t *count){struct test *t=v;assert(ch==14&&sl==5&&!memcmp(s,"Name?",5));assert(cap>=4);memcpy(buf,"TEST",4);*count=4;t->r++;return 1;}
static uint32_t writeq(void *v,uint16_t ch,const uint8_t *b,uint16_t n){struct test *t=v;assert(ch==14 && n<sizeof t->last);memcpy(t->last,b,n);t->last[n]=0;t->w++;return 1;}
int main(void){struct test t={0};struct port_boo_readprompt io={0};io.context=&t;io.assign=assign;io.qiow_readprompt=readp;io.qiow_write=writeq;
uint8_t buf[16]={0};assert(port_boo_readprompt(&io,"Name?",12,buf)==1);assert(buf[0]==4&&!memcmp(buf+1,"TEST",4));assert(t.a==1&&t.r==1);
assert(port_boo_readprompt(&io,"Done",0,NULL)==1);assert(t.a==1&&t.w==1&&!strcmp(t.last,"Done"));
memcpy(io.output,"OUTPUT",6);io.output_length=6;assert(port_boo_output_line(&io)==1);assert(t.w==2&&!strcmp(t.last,"OUTPUT\r\n"));
assert(!io.output[8]&&!io.output[9]);io.output_length=253;assert(port_boo_output_line(&io)==PORT_SS_BADPARAM);
puts("PASS: standalone SYSGEN prompt QIO, channel reuse and output line");}
