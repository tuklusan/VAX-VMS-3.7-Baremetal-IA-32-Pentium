#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include "vms_ini_log_templates.h"
#include "vms_ini_syslog.h"
#define CHECK(x) do{if(!(x)){fprintf(stderr,"FAIL %s:%d %s\n",__FILE__,__LINE__,#x);return 1;}}while(0)
int main(void){
    uint8_t a[48],b[48],p[128] __attribute__((aligned(16)));
    struct port_ini_syslog_result r={0};
    struct port_ini_syslog_settings s={.size_field_offset=8u,.pool_address=0x320000u,.allocation_alignment=16u};
    memset(p,0xa5,sizeof p);
    port_ini_log_templates(a,b);
    CHECK(a[10]==PORT_V37_DYN_LOG && b[10]==PORT_V37_DYN_LOG && a[11]==0u && b[11]==0u);
    CHECK(a[8]==48u && b[8]==48u);
    CHECK(a[12]==1u && b[12]==1u && a[14]==1u && b[14]==1u);
    CHECK(a[20]==8u && b[20]==13u && !memcmp(a+21,"SYS$DISK",8u));
    CHECK(!memcmp(b+21,"SYS$SYSDEVICE",13u));
    CHECK(a[29]==5u && !memcmp(a+30,"_DXX000:",8u));
    CHECK(b[34]==1u && !memcmp(b+35,"__DDCU:::",9u));
    for(unsigned i=40u;i<48u;i++)CHECK(a[i]==0u);
    for(unsigned i=44u;i<48u;i++)CHECK(b[i]==0u);
    CHECK(port_ini_syslog_init(p,95u,a,sizeof a,b,sizeof b,&s,&r)==PORT_INI_SYSLOG_NOMEM);
    CHECK(p[0]==0xa5u);
    CHECK(port_ini_syslog_init(p,sizeof p,a,sizeof a,b,sizeof b,&s,&r)==PORT_INI_SYSLOG_OK);
    CHECK(r.consumed_bytes==96u && r.sizes[0]==48u && r.sizes[1]==48u);
    CHECK(r.logical_table[0]==s.pool_address && r.logical_table[1]==s.pool_address+48u);
    CHECK(!memcmp(r.records[0],a,48u) && !memcmp(r.records[1],b,48u) && p[96]==0xa5u);
    puts("INIT.MAR source-sized 48-byte logical-name block templates PASS");
    return 0;
}
