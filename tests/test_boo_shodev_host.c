#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "vms_boo_shodev.h"
struct rig {unsigned lines,change;char data[16][128];};
static uint32_t change(void *p){struct rig *r=p;r->change++;return 1;}
static uint32_t line(void *p,const char *s,uint16_t n){struct rig *r=p;assert(r->lines<16&&n<128);memcpy(r->data[r->lines],s,n);r->data[r->lines++][n]=0;return 1;}
int main(void){
struct rig r={0};struct port_boo_unit u={2,0x81230000u,0};
struct port_boo_devblock b={"DQA","DQDRIVER",0x8000,0x8010,0x8020,&u,0};
struct port_boo_driver d={"DQDRIVER",0x1000,0x200,&b,0};
struct port_boo_shodev s={&r,&d,change,line,0};
assert(port_boo_shodev(&s,0,0,0)==1);assert(r.lines==4&&r.change==1);
assert(strstr(r.data[1],"DQDRIVER")&&strstr(r.data[1],"00001000")&&strstr(r.data[1],"00001200"));
assert(strstr(r.data[2],"DQA")&&strstr(r.data[3],"81230000"));
r.lines=0;assert(port_boo_shodev(&s,"DQ",2,1)==1);assert(r.lines==2);
r.lines=0;assert(port_boo_shodev(&s,"ABC",3,1)==1);assert(r.lines==1);
puts("PASS: SYSGEN SHOW/DEVICE and SHOW/DRIVER traversal");}
