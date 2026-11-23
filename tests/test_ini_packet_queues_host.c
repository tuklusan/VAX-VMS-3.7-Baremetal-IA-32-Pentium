#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include "vms_ini_packet_queues.h"
#define CHECK(x) do {if(!(x)){fprintf(stderr,"FAIL %s:%d: %s\n",__FILE__,__LINE__,#x);return 1;}}while(0)
static uint32_t get32(const uint8_t *s){
    return (uint32_t)s[0]|((uint32_t)s[1]<<8)|((uint32_t)s[2]<<16)|((uint32_t)s[3]<<24);
}
static uint16_t get16(const uint8_t *s){return (uint16_t)((uint16_t)s[0]|((uint16_t)s[1]<<8));}
int main(void){
    uint8_t data[1024];
    memset(data,0xa5,sizeof data);
    struct port_ini_packet_area a={.memory=data,.address=0x326000u,
        .available_bytes=sizeof data,.count=4u,.packet_bytes=176u,
        .minimum_bytes=176u,.sentinel_address=0x327000u};
    struct port_ini_packet_list q={0};
    CHECK(port_ini_packet_init(&a,&q)==PORT_INI_PACKETS_OK);
    CHECK(q.packet_count==4u && q.packet_bytes==176u);
    CHECK(q.next==a.address+528u && q.previous==a.address);
    CHECK(port_ini_packet_first(&q,a.sentinel_address)==q.next);
    for(unsigned i=0;i<4u;i++){
        size_t off=i*176u;
        CHECK(get32(&data[off])==(i? a.address+(i-1u)*176u:a.sentinel_address));
        CHECK(get32(&data[off+4u])==(i<3u?a.address+(i+1u)*176u:a.sentinel_address));
        CHECK(get16(&data[off+8u])==176u);
        CHECK(data[off+11u]==0xa5);
    }
    CHECK(q.remainder_address==a.address+704u && q.remainder_bytes==320u);
    CHECK(get32(&data[704])==320u && data[708]==0xa5);
    struct port_ini_packet_list saved=q;
    uint8_t snapshot[1024];memcpy(snapshot,data,sizeof data);
    a.available_bytes=703u;
    CHECK(port_ini_packet_init(&a,&q)==PORT_INI_PACKETS_BADPARAM);
    CHECK(!memcmp(data,snapshot,sizeof data) && !memcmp(&q,&saved,sizeof q));
    a.available_bytes=sizeof data;
    a.sentinel_address=a.address+176u;
    CHECK(port_ini_packet_init(&a,&q)==PORT_INI_PACKETS_BADPARAM);
    a.sentinel_address=0x327000u;a.packet_bytes=175u;
    CHECK(port_ini_packet_init(&a,&q)==PORT_INI_PACKETS_BADPARAM);
    a.packet_bytes=176u;a.count=0u;a.available_bytes=0u;a.memory=NULL;
    CHECK(port_ini_packet_init(&a,&q)==PORT_INI_PACKETS_OK);
    CHECK(q.next==a.sentinel_address && q.previous==a.sentinel_address);
    CHECK(!port_ini_packet_first(&q,a.sentinel_address));
    CHECK(q.remainder_address==0u && q.remainder_bytes==0u);
    a.count=UINT32_MAX;a.available_bytes=sizeof data;a.memory=data;
    CHECK(port_ini_packet_init(&a,&q)==PORT_INI_PACKETS_BADPARAM);
    puts("original INIT.MAR INSQUE IRP/LRP/SRP packet headers and partial-page remainder PASS");
    return 0;
}
