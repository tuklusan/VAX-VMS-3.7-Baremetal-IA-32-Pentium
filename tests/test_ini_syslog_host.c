#include "vms_ini_syslog.h"
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#define CHECK(x) do{if(!(x)){fprintf(stderr,"FAIL %s:%d %s\n",__FILE__,__LINE__,#x);return 1;}}while(0)
static void w16(uint8_t *p,uint16_t v){p[0]=(uint8_t)v;p[1]=(uint8_t)(v>>8);}
int main(void){
    uint8_t pool[256] __attribute__((aligned(16)));
    uint8_t a[32],b[32];
    memset(pool,0xa5,sizeof pool);memset(a,0x44,sizeof a);memset(b,0x99,sizeof b);
    w16(a+8u,27u);w16(b+8u,21u);
    memcpy(a+10u,"SYS$DISK",8);memcpy(b+10u,"SYS$SYSDEV",10);
    struct port_ini_syslog_settings s={8u,0x320000u,8u};
    struct port_ini_syslog_result o={0};
    CHECK(port_ini_syslog_init(pool,36u,a,sizeof a,b,sizeof b,&s,&o)==PORT_INI_SYSLOG_NOMEM);
    CHECK(pool[0]==0xa5u&&o.consumed_bytes==0u);
    CHECK(port_ini_syslog_init(pool,sizeof pool,a,sizeof a,b,sizeof b,&s,&o)==PORT_INI_SYSLOG_OK);
    CHECK(o.logical_table[0]==0x320000u&&o.logical_table[1]==0x320020u);
    CHECK(o.records[0]==pool&&o.records[1]==pool+32u);
    CHECK(o.sizes[0]==27u&&o.sizes[1]==21u&&o.consumed_bytes==53u);
    CHECK(!memcmp(pool,a,27u)&&!memcmp(pool+32u,b,21u));
    for(unsigned i=27u;i<32u;i++)CHECK(pool[i]==0u);
    CHECK(pool[53u]==0xa5u&&pool[255u]==0xa5u);
    w16(b+8u,33u);
    CHECK(port_ini_syslog_init(pool,sizeof pool,a,sizeof a,b,sizeof b,&s,&o)==PORT_INI_SYSLOG_BADPARAM);
    CHECK(pool[53u]==0xa5u);
    w16(b+8u,21u);
    s.allocation_alignment=3u;
    CHECK(port_ini_syslog_init(pool,sizeof pool,a,sizeof a,b,sizeof b,&s,&o)==PORT_INI_SYSLOG_BADPARAM);
    s.allocation_alignment=8u;s.pool_address=0xffffffd0u;
    CHECK(port_ini_syslog_init(pool,sizeof pool,a,sizeof a,b,sizeof b,&s,&o)==PORT_INI_SYSLOG_NOMEM);
    s.pool_address=0x320000u;
    CHECK(port_ini_syslog_init(pool,sizeof pool,pool,sizeof pool,b,sizeof b,&s,&o)==PORT_INI_SYSLOG_BADPARAM);
    CHECK(port_ini_syslog_init(pool,sizeof pool,a,sizeof a,pool,sizeof pool,&s,&o)==PORT_INI_SYSLOG_BADPARAM);
    CHECK(port_ini_syslog_init(pool,sizeof pool,a,sizeof a,b,sizeof b,&s,NULL)==PORT_INI_SYSLOG_BADPARAM);
    puts("INIT.MAR source-defined SYS$DISK and SYS$SYSDEVICE nonpaged records PASS");
    return 0;
}
