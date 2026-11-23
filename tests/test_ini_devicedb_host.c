#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include "vms_ini_devicedb.h"
#define CHECK(x) do {if(!(x)){fprintf(stderr,"FAIL %s:%d: %s\n",__FILE__,__LINE__,#x);return 1;}}while(0)
int main(void){
    uint32_t words[PORT_V37_DEVICE_BYTES/4u+4u];
    uint8_t *const m=(uint8_t *)words;
    const uint32_t b=0x00270000u;
    struct port_v37_device_result r={0};
    memset(words,0xa5,sizeof words);
    CHECK(port_v37_devices_prelink(m,PORT_V37_DEVICE_BYTES-1u,b,&r)==PORT_V37_DEV_BAD);
    for(unsigned i=0;i<sizeof words;i++)CHECK(m[i]==0xa5u);
    CHECK(port_v37_devices_prelink(m,sizeof words,b+1u,&r)==PORT_V37_DEV_BAD);
    for(unsigned i=0;i<sizeof words;i++)CHECK(m[i]==0xa5u);
    CHECK(port_v37_devices_prelink(m,sizeof words,0xfffffd00u,&r)==PORT_V37_DEV_BAD);
    CHECK(port_v37_devices_prelink(m+1u,sizeof words-1u,b,&r)==PORT_V37_DEV_BAD);
    CHECK(port_v37_devices_prelink(m,sizeof words,b,&r)==PORT_V37_DEV_OK);
    CHECK(port_v37_devices_check(m,sizeof words,b,&r)==PORT_V37_DEV_OK);
    CHECK(r.list_head==r.ddb[0] && r.ddb[3]==b+156u);
    CHECK(r.ucb[0]==b+208u && r.ucb[1]==b+460u && r.ucb[4]==b+1044u);
    CHECK(r.incomplete_driver_mask==15u);
    for(unsigned i=PORT_V37_DEVICE_BYTES;i<sizeof words;i++)CHECK(m[i]==0xa5u);
    CHECK(port_v37_read16(m+PORT_V37_UCB_OPA_OFF+8u)==320u);
    CHECK(port_v37_read16(m+PORT_V37_UCB_BOOT_OFF+8u)==252u);
    CHECK(port_v37_read16(m+PORT_V37_UCB_MB1_OFF+8u)==132u);
    CHECK(port_v37_read16(m+PORT_V37_UCB_MB1_OFF+72u)==1u);
    CHECK(port_v37_read16(m+PORT_V37_UCB_MB2_OFF+72u)==2u);
    CHECK(port_v37_read16(m+PORT_V37_UCB_MB1_OFF+90u)==0x8001u);
    CHECK(port_v37_read16(m+PORT_V37_UCB_MB2_OFF+90u)==1u);
    CHECK(port_v37_read16(m+PORT_V37_UCB_OPA_OFF+240u)==132u);
    CHECK(port_v37_read32(m+PORT_V37_DDB_BOOT_OFF+12u)==0u);
    CHECK(port_v37_read32(m+PORT_V37_DDB_OPA_OFF+12u)==0u);
    CHECK(port_v37_read32(m+PORT_V37_DDB_MB_OFF+12u)==0u);
    CHECK(port_v37_read32(m+PORT_V37_DDB_NL_OFF+12u)==0u);
    for(unsigned i=0;i<5u;i++)CHECK(port_v37_read32(m+(r.ucb[i]-b)+120u)==0u);
    CHECK(port_v37_devices_check(m,sizeof words,b,&r)==PORT_V37_DEV_OK);
    const uint32_t old=port_v37_read32(m+PORT_V37_UCB_MB1_OFF+44u);
    port_v37_write32(m+PORT_V37_UCB_MB1_OFF+44u,0u);
    CHECK(port_v37_devices_check(m,sizeof words,b,&r)==PORT_V37_DEV_BAD);
    port_v37_write32(m+PORT_V37_UCB_MB1_OFF+44u,old);
    CHECK(port_v37_devices_check(m,sizeof words,b,&r)==PORT_V37_DEV_OK);
    m[PORT_V37_DDB_OPA_OFF+10u]=PORT_V37_DYN_UCB;
    CHECK(port_v37_devices_check(m,sizeof words,b,&r)==PORT_V37_DEV_BAD);
    m[PORT_V37_DDB_OPA_OFF+10u]=PORT_V37_DYN_DDB;
    CHECK(port_v37_devices_check(m,sizeof words,b,&r)==PORT_V37_DEV_OK);
    puts("DEVICEDAT.MAR original DDB/UCB nonpublished prelink bytes and pointer chains PASS");
    return 0;
}
