#include <stdint.h>
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "vms_userptr.h"
static uint32_t pd[1024],pt0[1024],pt1[1024];
static uint8_t pa[4096],pb[4096];
static const uint32_t *table(void *ctx,uint32_t addr){
    (void)ctx;
    return addr==0x5000 ? pt0 : addr==0x6000 ? pt1 : 0;
}
static uint8_t *physical(void *ctx,uint32_t addr){
    (void)ctx;
    return addr==0xa000 ? pa : addr==0x9000 ? pb : 0;
}
int main(void){
    struct port_user_map m={pd,table,0};
    pd[0]=0x5000|7u;
    pt0[1]=0xa000|7u;
    pt0[2]=0x9000|7u;
    uint8_t src[32];for(int i=0;i<32;i++)src[i]=(uint8_t)(i+1);
    memset(pa,0,sizeof(pa));memset(pb,0,sizeof(pb));
    assert(port_user_copy_to(&m,0x1ff0,src,sizeof(src),physical,0)==0);
    assert(memcmp(pa+4080,src,16)==0 && memcmp(pb,src+16,16)==0);
    assert(pa[4079]==0 && pb[16]==0);
    /* Missing physical translation fails before the first byte is written. */
    memset(pa,0x33,sizeof(pa));
    pt0[2]=0xb000|7u;
    assert(port_user_copy_to(&m,0x1ff0,src,sizeof(src),physical,0)==PORT_USER_BAD);
    assert(pa[4080]==0x33 && pa[4095]==0x33);
    pt0[2]=0x9000|7u;
    pt0[2]=0x9000|5u;
    assert(port_user_copy_to(&m,0x1ff0,src,sizeof(src),physical,0)==PORT_USER_BAD);
    assert(pa[4080]==0x33);
    pt0[2]=0x9000|7u;
    assert(port_user_copy_to(&m,0,src,4,physical,0)==PORT_USER_BAD);
    assert(port_user_copy_to(&m,0xfffffff8u,src,32,physical,0)==PORT_USER_BAD);
    assert(port_user_copy_to(&m,0x1000,0,4,physical,0)==PORT_USER_BAD);
    assert(port_user_copy_to(&m,0x1000,src,0,physical,0)==PORT_USER_BAD);
    assert(port_user_copy_to(&m,0x1000,src,8193,physical,0)==PORT_USER_BAD);
    /* Crossing a PDE boundary must honor the next page table. */
    pd[1]=0x6000|7u;pt0[1023]=0xa000|7u;pt1[0]=0x9000|7u;
    assert(port_user_copy_to(&m,0x3ffff0,src,32,physical,0)==PORT_USER_OK);
    assert(memcmp(pa+4080,src,16)==0 && memcmp(pb,src+16,16)==0);
    puts("M30 translated user copy, PDE crossings and atomic failure PASS");
    return 0;
}
