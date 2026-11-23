#include "vms_boot_readprompt.h"
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

struct test_io {
    unsigned assigns, writes, reads, mode;
    uint16_t channel;
    char last[280];
    unsigned last_length;
    uint32_t status, iosb;
};
static uint32_t assign(void *p,const char *dev,unsigned mode,uint16_t *channel){
    struct test_io *s=p;
    assert(strcmp(dev,"_OPA0")==0);
    s->assigns++;s->mode=mode;*channel=42;
    return s->status;
}
static uint32_t write_console(void *p,uint16_t channel,const char *msg,
                              unsigned n,uint16_t *iosb){
    struct test_io *s=p;
    assert(channel==42 && n<sizeof s->last);
    s->writes++;s->channel=channel;s->last_length=n;
    memcpy(s->last,msg,n);s->last[n]=0;*iosb=(uint16_t)s->iosb;
    return s->status;
}
static uint32_t read_console(void *p,uint16_t channel,char *dst,unsigned cap,
                             const char *prompt,unsigned n,uint16_t *iosb,
                             uint16_t *received){
    struct test_io *s=p;
    assert(channel==42 && cap>=4u && n==9u);
    assert(memcmp(prompt,"SYSBOOT> ",9)==0);
    s->reads++;memcpy(dst,"DUA0",4);*received=4;*iosb=(uint16_t)s->iosb;
    return s->status;
}
int main(void){
    struct test_io state={.status=1,.iosb=1};
    const struct port_boo_console_io ops={assign,write_console,read_console};
    struct port_boo_console c;
    uint8_t input[256]={0};
    port_boo_console_init(&c,&ops,&state);
    assert(port_boo_readprompt(&c,"SYSBOOT> ",4,input)==1);
    assert(input[0]==4 && memcmp(input+1,"DUA0",4)==0);
    assert(state.assigns==1 && state.reads==1 && state.mode==3);
    assert(port_boo_readprompt(&c,"Working",0,0)==1);
    assert(state.assigns==1 && state.writes==1 &&
           state.last_length==7 && !strcmp(state.last,"Working"));
    memcpy(c.outbuf,"SYSTEM",6);c.outlen=6;
    assert(port_rio_output_line(&c)==1);
    assert(state.writes==2 && state.last_length==8 &&
           memcmp(state.last,"SYSTEM\r\n",8)==0);
    c.outlen=254;
    assert(port_rio_output_line(&c)==0);
    c.outlen=0;
    state.iosb=0;
    assert(port_boo_readprompt(&c,"FAIL",0,0)==0);
    state.iosb=1;
    state.status=0;
    assert(port_boo_readprompt(&c,"FAIL",0,0)==0);
    port_boo_console_init(&c,&ops,&state);
    assert(port_boo_readprompt(&c,"FAIL",0,0)==0);
    assert(state.assigns==2 && state.writes==4);
    assert(port_boo_readprompt(&c,0,0,0)==0);
    printf("PASS: BOO READPROMPT lazy _OPA0 channel, QIO IOSB, counted input, CRLF and failure paths.\n");
    return 0;
}
