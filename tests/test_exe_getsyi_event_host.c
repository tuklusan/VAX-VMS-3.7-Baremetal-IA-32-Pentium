#include <stdint.h>
#include <string.h>
#include <assert.h>
#include <stdio.h>
#include "vms_exe_getsyi.h"
#include "vms_exe_event.h"
struct live {
    struct port_exe_event_process events;
    uint8_t mem[0x5000];
    uint32_t posts;
};
static int read_mem(void *ctx,uint32_t at,void *to,uint32_t n){
    struct live *p=(struct live *)ctx;
    if(at<0x1000u||at>sizeof(p->mem)||n>sizeof(p->mem)-at)return 0;
    memcpy(to,p->mem+at,n);return 1;
}
static int write_mem(void *ctx,uint32_t at,const void *from,uint32_t n){
    struct live *p=(struct live *)ctx;
    if(at<0x1000u||at>sizeof(p->mem)||n>sizeof(p->mem)-at)return 0;
    memcpy(p->mem+at,from,n);return 1;
}
static int probe_mem(void *ctx,uint32_t at,uint32_t n){
    struct live *p=(struct live *)ctx;
    return at>=0x1000u&&at<=sizeof(p->mem)&&n<=sizeof(p->mem)-at;
}
static uint32_t clr(void *ctx,uint32_t flag){
    return port_exe_clref(&((struct live *)ctx)->events,flag);
}
static void post(void *ctx,uint32_t pid,uint32_t flag){
    struct live *p=(struct live *)ctx;
    assert(pid==p->events.pid);
    uint32_t status=port_exe_event_post(&p->events,flag);
    assert(status&1u);
    ++p->posts;
}
static uint32_t pd[1024],pt[1024];
static uint8_t pages[2][4096];
static const uint32_t *map_table(void *ctx,uint32_t at){
    (void)ctx;return at==0x5000u?pt:0;
}
static uint8_t *map_page(void *ctx,uint32_t at){
    (void)ctx;return at==0x8000u?pages[0]:at==0x9000u?pages[1]:0;
}
int main(void){
    struct live p={0};
    port_exe_event_init(&p.events,44u);
    struct port_syi_system system={.version={'S','1','J','Y',' ',' ',' ',' '}};
    struct port_syi_process proc={.pid=44u,.astcnt=2u,.previous_mode=3u};
    struct port_syi_request req={.efn=3u,.item_list=0x1100u,.iosb=0x1200u};
    struct port_syi_ops io={read_mem,write_mem,probe_mem,clr,post,0,&p};
    p.mem[0x1100]=8u;p.mem[0x1102]=0u;p.mem[0x1103]=1u;
    p.mem[0x1104]=0u;p.mem[0x1105]=0x13u;
    p.mem[0x1100+12u]=0;
    assert(port_exe_event_post(&p.events,3u)==PORT_SS_WASCLR);
    assert(port_exe_getsyi(&proc,&system,&io,&req)==PORT_SS_NORMAL);
    assert(p.posts==1u&&p.events.local[0]==8u);
    assert(!memcmp(p.mem+0x1300u,"S1JY    ",8u));
    assert(p.mem[0x1200u]==PORT_SS_NORMAL && p.mem[0x1204u]==0u);
    /* Failure also posts completion to the same real flag cluster. */
    req.item_list=0u;
    assert(port_exe_getsyi(&proc,&system,&io,&req)==PORT_SS_BADPARAM);
    assert(p.events.local[0]==8u&&p.posts==2u);
    req.item_list=0x1100u;

    struct port_user_map map={pd,map_table,0};
    struct port_syi_user u={&map,map_page,0};
    pd[0]=0x5007u;pt[1]=0x8007u;pt[2]=0x9007u;
    memset(pages,0xaa,sizeof(pages));
    assert(port_syi_user_probe(&u,0x1ff0u,32u));
    assert(port_syi_user_write(&u,0x1ffeu,"ABCD",4u));
    assert(pages[0][4094]=='A'&&pages[0][4095]=='B'&&pages[1][0]=='C');
    char value[4]={0};
    assert(port_syi_user_read(&u,0x1ffeu,value,4u));
    assert(!memcmp(value,"ABCD",4u));
    pt[2]=0x9005u;
    assert(!port_syi_user_probe(&u,0x1ff0u,32u));
    assert(!port_syi_user_write(&u,0x1ffeu,"TEST",4u));
    assert(pages[0][4094]=='A'&&pages[0][4095]=='B');
    pt[2]=0x9007u;
    assert(!port_syi_user_probe(&u,0xfffffff0u,64u));
    puts("SYSGETSYI with real SYSEVTSRV flags and IA-32 paging PASS");
    return 0;
}
