#include <stdint.h>
#include <stdio.h>
#include <assert.h>
#include "vms_userptr.h"
static uint32_t directory[1024],t0[1024],t1[1024];
static uint8_t pages[32][4096];
static unsigned pin_counts[32],pin_calls,fail_pin_call;
static uint8_t *physical(void *unused,uint32_t phys) {
    (void)unused;
    if((phys&4095u)||phys<0x1000u||phys>=0x21000u)return 0;
    return pages[phys>>12];
}
static int pin(void *unused,uint32_t phys) {
    (void)unused;
    ++pin_calls;
    if(pin_calls==fail_pin_call)return -1;
    ++pin_counts[phys>>12];
    return 0;
}
static void unpin(void *unused,uint32_t phys) {
    (void)unused;
    assert(pin_counts[phys>>12]);
    --pin_counts[phys>>12];
}
static const uint32_t *resolve(void *unused,uint32_t phys) {
    (void)unused;
    if(phys==0x5000)return t0;
    if(phys==0x6000)return t1;
    return 0;
}
int main(void) {
    struct port_user_map m={directory,resolve,0};
    directory[0]=0x5000|7u;
    t0[1]=0x1000|7u; t0[2]=0x2000|5u; t0[3]=0x3000|3u;
    assert(port_user_range_valid(&m,0x1001,4096,0)==PORT_USER_OK);
    assert(port_user_range_valid(&m,0x1000,4,1)==PORT_USER_OK);
    assert(port_user_range_valid(&m,0x2000,4,0)==PORT_USER_OK);
    assert(port_user_range_valid(&m,0x2000,4,1)==PORT_USER_BAD);
    assert(port_user_range_valid(&m,0x3000,4,0)==PORT_USER_BAD);
    assert(port_user_range_valid(&m,0x1ffe,4,0)==PORT_USER_OK);
    assert(port_user_range_valid(&m,0x1ffe,4,1)==PORT_USER_BAD);
    assert(port_user_range_valid(&m,0,8,0)==PORT_USER_BAD);
    assert(port_user_range_valid(&m,0xfffffffeu,4,0)==PORT_USER_BAD);
    assert(port_user_range_valid(&m,0x1000,0,0)==PORT_USER_BAD);
    assert(port_user_range_valid(&m,0x1000,8193,0)==PORT_USER_BAD);
    assert(port_user_range_valid(&m,0x4000,4,0)==PORT_USER_BAD);
    t0[3]=0x3000|7u;
    assert(port_user_range_valid(&m,0x3000,4,1)==PORT_USER_OK);
    directory[0]=0x5000|5u;
    assert(port_user_range_valid(&m,0x1000,4,1)==PORT_USER_BAD);
    assert(port_user_range_valid(&m,0x1000,4,0)==PORT_USER_OK);
    directory[0]=0x5000|7u;
    directory[1]=0x6000|7u;
    t0[1023]=0x400000|7u;
    t1[0]=0x400000|7u;
    assert(port_user_range_valid(&m,0x3ffffe,4,1)==PORT_USER_OK);
    t1[0]=0x400000|3u;
    assert(port_user_range_valid(&m,0x3ffffe,4,0)==PORT_USER_BAD);
    t1[0]=0x400000|7u;
    directory[1]=0x6000|3u;
    assert(port_user_range_valid(&m,0x3ffffe,4,0)==PORT_USER_BAD);
    directory[1]=0xdead000|7u;
    assert(port_user_range_valid(&m,0x400000,4,0)==PORT_USER_BAD);
    directory[0]=0x5000|7u;
    for(unsigned i=1u;i<=17u;i++)t0[i]=((i+1u)<<12)|7u;
    assert(port_user_io_range_valid(&m,0x1001,UINT16_MAX,1)==PORT_USER_OK);
    t0[16]=0x11000u|5u;
    assert(port_user_io_range_valid(&m,0x1001,UINT16_MAX,1)==PORT_USER_BAD);
    assert(port_user_io_range_valid(&m,0x1001,UINT16_MAX,0)==PORT_USER_OK);
    assert(port_user_io_range_valid(&m,0x1001,UINT16_MAX+1u,0)==PORT_USER_BAD);
    struct port_user_page_locks locks={0};
    assert(port_user_pin_io_range(&m,0x1002,UINT16_MAX,0,physical,0,
                                  pin,unpin,0,&locks)==PORT_USER_OK);
    assert(locks.count==17u&&pin_counts[2]==1u&&pin_counts[18]==1u);
    port_user_unpin_io_range(&locks);
    assert(locks.count==0u&&pin_counts[2]==0u&&pin_counts[18]==0u);
    locks.physical[0]=0xabcdef00u;
    pin_calls=0;fail_pin_call=4;
    assert(port_user_pin_io_range(&m,0x1002,UINT16_MAX,0,physical,0,
                                  pin,unpin,0,&locks)==PORT_USER_BAD);
    assert(locks.count==0u&&locks.physical[0]==0xabcdef00u);
    for(unsigned i=0;i<32;i++)assert(pin_counts[i]==0u);
    fail_pin_call=0;
    puts("M293 user pointer access, crossings, IO page pin rollback PASS");
    return 0;
}
