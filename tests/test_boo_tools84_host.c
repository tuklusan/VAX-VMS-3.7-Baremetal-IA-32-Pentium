#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include "vms_boo_actimage.h"
#include "vms_rtf_readlbn.h"
#include "vms_iogen_readdriv.h"
#include "vms_boo_conio.h"
struct rig {uint8_t mem[512],input[64],output[128];uint32_t pos,count,sequence,opened,read,closed,signalled;};
static int memread(void *p,uint32_t addr,void *out,uint32_t n){struct rig *r=p;if(addr>512||n>512-addr)return 0;memcpy(out,r->mem+addr,n);return 1;}
static int memwrite(void *p,uint32_t addr,const void *data,uint32_t n){struct rig *r=p;if(addr>512||n>512-addr)return 0;memcpy(r->mem+addr,data,n);return 1;}
static int probe(void *p,uint32_t addr,uint32_t n){(void)p;return addr<=512&&n<=512-addr;}
static uint32_t mode(void *p){struct rig *r=p;assert(r->sequence++==0);return 1;}
static uint32_t stack(void *p){struct rig *r=p;assert(r->sequence++==1);return 1;}
static uint32_t rundown(void *p){struct rig *r=p;assert(r->sequence++==2);return 1;}
static uint32_t activate(void *p,const uint8_t *name,uint16_t len){struct rig *r=p;assert(r->sequence++==3);assert(len==7&&!memcmp(name,"SYS.EXE",7));return 1;}
static uint32_t assign(void *p,const uint8_t *name,uint16_t size,uint16_t *ch){struct rig *r=p;assert(size==4&&!memcmp(name,"DQA0",4));r->opened++;*ch=7;return 1;}
static uint32_t readblk(void *p,uint16_t ch,uint32_t lbn,uint8_t data[512],uint16_t *iosb){struct rig *r=p;assert(ch==7&&lbn==42);r->read++;memset(data,0x5a,512);*iosb=1;return 1;}
static uint32_t deassign(void *p,uint16_t ch){struct rig *r=p;assert(ch==7);r->closed++;return 1;}
static uint32_t openimage(void *p,const char *name,uint16_t len,uint16_t *ch){struct rig *r=p;assert(len==8&&!memcmp(name,"DQDRIVER",8));r->opened++;*ch=4;return 1;}
static uint32_t mapimage(void *p,uint16_t ch,uint32_t range[2]){struct rig *r=p;assert(ch==4);range[0]=0x4000;range[1]=0x5fff;r->read++;return 1;}
static void signal(void *p,uint32_t status){(void)status;((struct rig *)p)->signalled++;}
static int available(void *p){(void)p;return 0;}
static int readchar(void *p,uint8_t *c){struct rig *r=p;if(r->pos>=r->count)return 0;*c=r->input[r->pos++];return 1;}
static int writechar(void *p,uint8_t c){struct rig *r=p;assert(r->sequence<128);r->output[r->sequence++]=c;return 1;}
int main(void){
 struct rig r={0};
 struct port_exe_access mem={&r,memread,memwrite,probe,probe};
 uint8_t d[8]={7,0,0,0,100,0,0,0};assert(memwrite(&r,32,d,8));
 assert(memwrite(&r,100,"SYS.EXE",7));
 struct port_boo_actimage_ops ai={&r,&mem,mode,stack,rundown,activate};
 assert(port_boo_actimage(32,&ai)==1 && r.sequence==4);
 assert(port_boo_actimage(510,&ai)==PORT_SS_ACCVIO);
 struct port_rtf_target target={0};
 assert(port_rtf_target_dev(&target,(const uint8_t *)"DQA0",4)==1);
 assert(port_rtf_target_dev(&target,(const uint8_t *)"DEVICE-NAME-TOO-LONG",20)==1);
 assert(port_rtf_target_dev(&target,(const uint8_t *)"TOO-LONG-TARGET-DEVICE",22)==PORT_SS_BADPARAM);
 assert(port_rtf_target_dev(&target,(const uint8_t *)"DQA0",4)==1);
 struct port_rtf_read_ops io={&r,assign,readblk,deassign};
 uint8_t data[512]={0};
 assert(port_rtf_read_lbn(&target,42,data,&io)==1);
 assert(data[0]==0x5a&&data[511]==0x5a&&r.closed==1);
 struct port_iogen_readdriv_ops drv={&r,openimage,mapimage,signal};
 uint8_t name[10]={8,'D','Q','D','R','I','V','E','R',0};
 uint16_t ch=0;uint32_t range[2]={0};
 assert(port_iogen_readdriv(name,0,&ch,range,&drv)==1);
 assert(ch==4&&range[0]==0x4000&&range[1]==0x5fff);
 struct port_boo_console_ops cons={&r,available,readchar,writechar};
 r.sequence=0;r.pos=0;
 r.input[0]='h';r.input[1]='e';r.input[2]=127;r.input[3]='l';r.input[4]='l';r.input[5]=13;r.count=6;
 uint8_t counted[32]={0};
 assert(port_boo_conio_prompt("Boot> ",32,counted,&cons)==1);
 assert(counted[0]==3&&!memcmp(counted+1,"HLL",3));
 assert(r.output[0]=='B'&&r.output[1]=='o');
 assert(port_boo_conio_prompt("",0,NULL,&cons)==1);
 puts("PASS: original ACTIMAGE, READLBN, READDRIV and CONIO paths");
}
