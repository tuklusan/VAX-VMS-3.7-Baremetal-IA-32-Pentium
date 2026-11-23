#include <stdint.h>
#include <stdio.h>
#include <assert.h>
#include "vms_userptr.h"
static uint32_t directory[1024],t0[1024],t1[1024];
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
    puts("M27 user pointer PDE/PTE ownership, access, crossings, overflow PASS");
    return 0;
}
