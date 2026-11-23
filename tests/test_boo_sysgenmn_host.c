#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "vms_boo_sysgenmn.h"
struct test {unsigned calls;uint8_t verify;uint32_t status;};
static uint32_t lock(void *p){struct test *t=p;assert(t->calls++==0);return 1;}
static uint32_t cli(void *p,uint8_t *st){struct test *t=p;assert(t->calls++==1);*st=2;return 1;}
static uint32_t active(void *p){struct test *t=p;assert(t->calls++==2);return 1;}
static uint32_t commands(void *p,uint8_t v){struct test *t=p;assert(t->calls++==3);t->verify=v;return t->status;}
static uint32_t putmsg(void *p,uint32_t st,const char *fac){struct test *t=p;assert(st==12&&!strcmp(fac,"SYSGEN"));t->calls++;return 1;}
int main(void){
 struct test t={0};struct port_boo_sysgen s={0};
 s.context=&t;s.lock_image=lock;s.cli_utility=cli;s.use_active=active;s.read_commands=commands;s.putmsg=putmsg;
 s.foreign_verb=2;s.command_length=7;s.command_address=100;s.eof_status=40;
 t.status=40;assert(port_boo_sysgen_start(&s,4)==1);assert(t.calls==4&&t.verify==1&&s.command_length==7);
 assert(port_boo_sysgen_puterror(&s,12,"SYSGEN")==1&&t.calls==5);
 t.calls=0;t.status=12;s.foreign_verb=3;
 assert(port_boo_sysgen_start(&s,0)==12);assert(t.verify==0&&!s.command_length&&!s.command_address);
 puts("PASS: SYSGEN startup, CLI request, ACTIVE values, EOF and message");
}
