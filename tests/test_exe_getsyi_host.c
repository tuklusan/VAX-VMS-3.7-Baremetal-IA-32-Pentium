#include <stdint.h>
#include <string.h>
#include <assert.h>
#include <stdio.h>
#include "vms_exe_getsyi.h"
static uint8_t mem[0x7000];
static uint32_t clear_count,post_count,ast_count,last_status,last_param,last_mode;
static uint32_t posted_pid,posted_flag;
static uint32_t event_status=PORT_SS_NORMAL;
static int raw_read(void *ctx,uint32_t at,void *out,uint32_t n) {
    (void)ctx;
    if(at<0x1000u||at>sizeof(mem)||n>sizeof(mem)-at)return 0;
    memcpy(out,mem+at,n);return 1;
}
static int raw_probe(void *ctx,uint32_t at,uint32_t n) {
    (void)ctx;
    return at>=0x1000u&&at<=sizeof(mem)&&n<=sizeof(mem)-at;
}
static int raw_write(void *ctx,uint32_t at,const void *in,uint32_t n) {
    if(!raw_probe(ctx,at,n))return 0;
    memcpy(mem+at,in,n);return 1;
}
static uint32_t clear_event(void *ctx,uint32_t flag) {
    (void)ctx;assert(flag==7u);clear_count++;return event_status;
}
static void post_event(void *ctx,uint32_t pid,uint32_t flag) {
    (void)ctx;posted_pid=pid;posted_flag=flag;post_count++;
}
static void post_ast(void *ctx,uint32_t address,uint32_t param,uint32_t mode) {
    (void)ctx;assert(address==0xfeedu);ast_count++;
    last_param=param;last_mode=mode;
}
static struct port_syi_ops ops={raw_read,raw_write,raw_probe,
                                clear_event,post_event,post_ast,0};
static struct port_syi_process process={.pid=42u,.astcnt=2u,.previous_mode=3u};
static struct port_syi_system system={.version={'S','1','J','Y',' ',' ',' ',' '},
                                     .cpu_type=0x91u,.sid=0x12345678u};
static struct port_syi_request request={.efn=7u,.item_list=0x1000u,.iosb=0x6000u};
static void item(unsigned n,uint16_t capacity,uint16_t code,
                 uint32_t buffer,uint32_t length) {
    uint32_t at=request.item_list+12u*n;
    mem[at]=(uint8_t)capacity;mem[at+1]=(uint8_t)(capacity>>8);
    mem[at+2]=(uint8_t)code;mem[at+3]=(uint8_t)(code>>8);
    for(unsigned j=0;j<4u;j++){
        mem[at+4u+j]=(uint8_t)(buffer>>(j*8u));
        mem[at+8u+j]=(uint8_t)(length>>(j*8u));
    }
}
static uint32_t word(uint32_t addr) {
    uint32_t result=0;
    for(unsigned j=0;j<4u;j++)result|=(uint32_t)mem[addr+j]<<(8u*j);
    return result;
}
static void base(void) {
    memset(mem,0xcc,sizeof(mem));
    clear_count=post_count=ast_count=last_status=0;
    posted_flag=posted_pid=last_param=last_mode=0;
    event_status=PORT_SS_NORMAL;
    request=(struct port_syi_request){.efn=7u,.iosb=0x6000u,.item_list=0x1000u};
    process.astcnt=2u;
}
int main(void) {
    base();
    item(0,8,PORT_SYI_VERSION,0x3000u,0x4100u);
    item(1,4,PORT_SYI_CPUTYPE,0x3010u,0x4102u);
    item(2,2,PORT_SYI_SID,0x3020u,0x4104u);
    item(3,0,0,0,0);
    request.ast_address=0xfeedu;request.ast_parameter=0xbaadbeefu;
    assert(port_exe_getsyi(&process,&system,&ops,&request)==PORT_SS_NORMAL);
    assert(!memcmp(mem+0x3000u,"S1JY    ",8u));
    assert(mem[0x3010u]==0x91u && mem[0x3011u]==0 && mem[0x3013u]==0);
    assert(mem[0x3020u]==0x78u&&mem[0x3021u]==0x56u);
    assert(mem[0x4100u]==8u&&mem[0x4102u]==1u&&mem[0x4104u]==2u);
    assert(word(0x6000u)==PORT_SS_NORMAL && word(0x6004u)==0u);
    assert(clear_count==1u&&post_count==1u&&ast_count==1u);
    assert(posted_pid==42u&&posted_flag==7u&&last_param==0xbaadbeefu&&last_mode==3u);

    base();item(0,1,PORT_SYI_VERSION,0x3000u,0);item(1,0,0,0,0);
    assert(port_exe_getsyi(&process,&system,&ops,&request)==PORT_SS_NORMAL);
    assert(mem[0x3000u]=='S' && mem[0x3001u]==0xccu);

    base();item(0,4,0x0300u,0x3000u,0);item(1,0,0,0,0);
    assert(port_exe_getsyi(&process,&system,&ops,&request)==PORT_SS_BADPARAM);
    assert(mem[0x3000u]==0xccu&&word(0x6000u)==PORT_SS_BADPARAM);
    assert(clear_count==1u&&post_count==1u);

    base();item(0,4,PORT_SYI_SID,0x3000u,0);
    request.item_list=0u;
    assert(port_exe_getsyi(&process,&system,&ops,&request)==PORT_SS_BADPARAM);
    assert(post_count==1u);

    base();item(0,4,PORT_SYI_SID,0x3000u,0);item(1,0,0,0,0);
    request.ast_address=0xfeedu;process.astcnt=0u;
    assert(port_exe_getsyi(&process,&system,&ops,&request)==PORT_SS_EXASTLM);
    assert(mem[0x3000u]==0xccu && ast_count==1u && post_count==1u);

    base();item(0,4,PORT_SYI_SID,0x3000u,0);item(1,0,0,0,0);
    event_status=PORT_SS_ILLEFC;
    assert(port_exe_getsyi(&process,&system,&ops,&request)==PORT_SS_ILLEFC);
    assert(mem[0x3000u]==0xccu && post_count==1u);

    base();item(0,4,PORT_SYI_SID,0x7000u,0);item(1,0,0,0,0);
    assert(port_exe_getsyi(&process,&system,&ops,&request)==PORT_SS_ACCVIO);
    assert(post_count==1u && word(0x6000u)==PORT_SS_ACCVIO);

    base();item(0,4,PORT_SYI_SID,0x3000u,0x7000u);item(1,0,0,0,0);
    assert(port_exe_getsyi(&process,&system,&ops,&request)==PORT_SS_ACCVIO);
    assert(mem[0x3000u]==0x78u && post_count==1u);

    base();request.item_list=0x6ff4u;
    item(0,4,PORT_SYI_SID,0x3000u,0);
    assert(port_exe_getsyi(&process,&system,&ops,&request)==PORT_SS_ACCVIO);
    assert(mem[0x3000u]==0xccu && post_count==1u);

    base();request.iosb=0x7000u;
    assert(port_exe_getsyi(&process,&system,&ops,&request)==PORT_SS_ACCVIO);
    assert(clear_count==0u && post_count==1u);

    /* A valid first span and denied later span must write nothing. */
    base();item(0,0x100u,PORT_SYI_VERSION,0x6f80u,0);
    item(1,0,0,0,0);
    assert(port_exe_getsyi(&process,&system,&ops,&request)==PORT_SS_ACCVIO);
    assert(mem[0x6f80u]==0xccu);

    puts("SYSGETSYI complete source service path tests PASS");
    return 0;
}
