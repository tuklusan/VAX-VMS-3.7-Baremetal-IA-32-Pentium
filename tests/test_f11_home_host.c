#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include "vms_f11_home.h"
static int failed;
#define CHECK(x) do {if(!(x)){fprintf(stderr,"FAIL %d: %s\n",__LINE__,#x);failed++;}}while(0)
static void set16(uint8_t *p,uint16_t v){p[0]=(uint8_t)v;p[1]=(uint8_t)(v>>8);}
static void set32(uint8_t *p,uint32_t v){set16(p,(uint16_t)v);set16(p+2,(uint16_t)(v>>16));}
int main(void) {
    uint8_t block[512]={0};
    set32(block,1);set32(block+8,85);set16(block+12,513);
    set16(block+14,3);set16(block+16,1);set16(block+18,2);
    set16(block+20,3);set16(block+22,4);set32(block+24,80);
    set32(block+28,128);set16(block+32,2);set16(block+34,16);
    memcpy(block+472,"ORIGINAL",8);
    memset(block+480,' ',4);
    int ods2=0;
    CHECK(!port_f11_checksum2(block,58));
    CHECK(!port_f11_checksum2(block,510));
    CHECK(port_f11_check_home2(block,1,"ORIGINAL",8,24,&ods2)==1u&&ods2==1);
    CHECK(port_f11_check_home2(block,1,"OTHER",5,24,&ods2)==24u);
    CHECK(port_f11_check_home2(block,1,"ORIGINAL",8,24,&ods2)==1u);
    CHECK(port_f11_check_home2(block,2,"ORIGINAL",8,24,&ods2)==0u);
    block[32] ^= 1u;
    CHECK(port_f11_check_home2(block,1,"ORIGINAL",8,24,&ods2)==0u);
    CHECK(block[58]!=0u || block[59]!=0u);
    CHECK(!port_f11_checksum2(block,59));
    CHECK(!port_f11_checksum2(block,512));
    CHECK(!port_f11_checksum2(0,510));
    uint8_t level1[512]={0};
    set16(level1+0,2u);set32(level1+2,40u);
    set16(level1+6,64u);set16(level1+8,3u);
    set16(level1+12,257u);
    memcpy(level1+14,"DISK1",5u);
    CHECK(!port_f11_checksum2(level1,58u));
    CHECK(!port_f11_checksum2(level1,510u));
    CHECK(port_f11_check_home1(level1,"DISK1",5u,24u,26u)==1u);
    CHECK(port_f11_check_home1(level1,"DISK2",5u,24u,26u)==24u);
    CHECK(port_f11_check_home1(level1,"DISK1 ",6u,24u,26u)==24u);
    set16(level1+12,258u);
    port_f11_checksum2(level1,58u);
    port_f11_checksum2(level1,510u);
    CHECK(port_f11_check_home1(level1,"DISK1",5u,24u,26u)==1u);
    set16(level1+12,512u);
    port_f11_checksum2(level1,58u);
    port_f11_checksum2(level1,510u);
    CHECK(port_f11_check_home1(level1,"DISK1",5u,24u,26u)==26u);
    set16(level1+12,257u);
    level1[14]^=1u;
    CHECK(port_f11_check_home1(level1,"DISK1",5u,24u,26u)==0u);
    level1[14]^=1u;
    set32(level1+2,0u);
    CHECK(port_f11_check_home1(level1,"DISK1",5u,24u,26u)==0u);
    CHECK(port_f11_check_home1(0,"DISK1",5u,24u,26u)==0u);
    if(failed)return 1;
    puts("PASS: CHECKSUM2 writeback, CHKHM1 ODS-1 and CHKHM2 ODS-2 checks");
    return 0;
}
