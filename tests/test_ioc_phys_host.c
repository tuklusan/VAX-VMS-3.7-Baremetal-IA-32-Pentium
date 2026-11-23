#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include "vms_ioc_phys.h"
#define CHECK(c) do{if(!(c)){fprintf(stderr,"FAIL %s:%d %s\n",__FILE__,__LINE__,#c);return 1;}}while(0)
static uint32_t lookup(const uint8_t *m,const struct port_v37_device_result *r,
    const char *name,size_t n,struct port_v37_ioc_result *out){
    return port_v37_ioc_searchdev(m,PORT_V37_DEVICE_BYTES,0x00320000u,r,name,n,out);
}
int main(void){
    uint32_t storage[PORT_V37_DEVICE_BYTES/4u]={0};
    uint8_t *m=(uint8_t *)storage;
    struct port_v37_device_result r={0};
    const uint32_t b=0x00320000u;
    CHECK(port_v37_devices_prelink(m,sizeof storage,b,&r)==PORT_V37_DEV_OK);
    struct port_v37_ioc_result got={.ddb=0xfacebad0u,.ucb=0x22222222u};
    CHECK(lookup(m,&r,"opa0:",5u,&got)==PORT_V37_IOC_UNBOUND);
    CHECK(got.ddb==r.ddb[1]&&got.ucb==r.ucb[1]&&got.unit==0u&&!got.driver_bound&&!got.concealed);
    CHECK(lookup(m,&r,"_OPA0:",6u,&got)==PORT_V37_IOC_UNBOUND&&got.concealed==0u);
    CHECK(lookup(m,&r,"__OPA0:",7u,&got)==PORT_V37_IOC_UNBOUND&&got.concealed==1u);
    CHECK(lookup(m,&r,"_OP0:",5u,&got)==PORT_V37_IOC_UNBOUND&&got.ucb==r.ucb[1]);
    CHECK(lookup(m,&r,"MBA1:",5u,&got)==PORT_V37_IOC_UNBOUND&&got.ucb==r.ucb[2]&&got.unit==1u);
    CHECK(lookup(m,&r,"mb2",3u,&got)==PORT_V37_IOC_UNBOUND&&got.ucb==r.ucb[3]&&got.unit==2u);
    CHECK(lookup(m,&r,"NLA0",4u,&got)==PORT_V37_IOC_UNBOUND&&got.ucb==r.ucb[4]&&got.unit==0u);
    CHECK(lookup(m,&r,"OPa",3u,&got)==PORT_V37_IOC_UNBOUND&&got.ucb==r.ucb[1]);
    struct port_v37_ioc_result before=got;
    CHECK(lookup(m,&r,"MBA3:",5u,&got)==PORT_V37_IOC_NO_SUCH);
    CHECK(!memcmp(&before,&got,sizeof got));
    CHECK(lookup(m,&r,"XXA0:",5u,&got)==PORT_V37_IOC_NO_SUCH);
    CHECK(lookup(m,&r,"a::OPa0",7u,&got)==PORT_V37_IOC_NONLOCAL);
    CHECK(lookup(m,&r,"SYS$DISK",8u,&got)==PORT_V37_IOC_LOGICAL_PENDING);
    CHECK(lookup(m,&r,"___OPA0",7u,&got)==PORT_V37_IOC_BAD);
    CHECK(lookup(m,&r,"OPA65536",8u,&got)==PORT_V37_IOC_BAD);
    CHECK(lookup(m,&r,"OPA-1",5u,&got)==PORT_V37_IOC_BAD);
    CHECK(lookup(m,&r,"OPA:4",5u,&got)==PORT_V37_IOC_BAD);
    CHECK(lookup(m,&r,"",0u,&got)==PORT_V37_IOC_BAD);
    CHECK(lookup(m,&r,"OPA0:\0X",7u,&got)==PORT_V37_IOC_BAD);
    CHECK(!memcmp(&before,&got,sizeof got));
    CHECK(port_v37_ioc_searchgen(m,sizeof storage,b,&r,"MBA",3u,&got)==PORT_V37_IOC_UNAVAILABLE);
    CHECK(port_v37_ioc_searchalc(m,sizeof storage,b,&r,"MBA",3u,&got)==PORT_V37_IOC_UNAVAILABLE);
    CHECK(!memcmp(&before,&got,sizeof got));
    CHECK(port_v37_ioc_validate(m,sizeof storage,b,&r)==PORT_V37_IOC_OK);
    const uint8_t old=m[PORT_V37_DDB_OPA_OFF+10u];
    m[PORT_V37_DDB_OPA_OFF+10u]=PORT_V37_DYN_UCB;
    CHECK(lookup(m,&r,"OPA0",4u,&got)==PORT_V37_IOC_BAD);
    m[PORT_V37_DDB_OPA_OFF+10u]=old;
    CHECK(lookup(m,&r,"OPA0",4u,&got)==PORT_V37_IOC_UNBOUND);
    CHECK(port_v37_ioc_searchdev(m,sizeof storage,b,&r,"OPA0",4u,0)==PORT_V37_IOC_BAD);
    CHECK(port_v37_ioc_upcase("aaa",3u,0,0u)==PORT_V37_IOC_BAD);
    CHECK(port_v37_ioc_parse_unit("999999",6u,&before.unit,&before.concealed)==PORT_V37_IOC_BAD);
    puts("IOSUBPAGD.MAR read-only physical device DDB/UCB lookup and fail-closed guards PASS");
    return 0;
}
