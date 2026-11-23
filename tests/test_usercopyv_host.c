#include <stdint.h>
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "vms_userptr.h"
static uint32_t pd[1024],a[1024],b[1024];
static uint8_t pa[4096],pb[4096],pc[4096];
static const uint32_t *table(void *ctx,uint32_t v){
 (void)ctx;return v==0x5000?a:v==0x6000?b:0;
}
static uint8_t *physical(void *ctx,uint32_t v){
 (void)ctx;return v==0xa000?pa:v==0xb000?pb:v==0xc000?pc:0;
}
int main(void){
 struct port_user_map m={pd,table,0};
 pd[0]=0x5000|7;pd[1]=0x6000|7;
 a[1]=0xa000|7;a[2]=0xb000|7;
 a[1023]=0xa000|7;b[0]=0xc000|7;
 uint8_t data[40];uint8_t iosb[12];
 for(unsigned i=0;i<sizeof(data);i++)data[i]=(uint8_t)(i+1u);
 memset(iosb,0x5c,sizeof(iosb));
 struct port_user_output output[2]={
   {0x1ff0,data,sizeof(data)},{0x2080,iosb,sizeof(iosb)}
 };
 memset(pa,0x77,sizeof(pa));memset(pb,0x77,sizeof(pb));
 assert(port_user_copyv_to(&m,output,2,physical,0)==0);
 assert(!memcmp(pa+4080,data,16));assert(!memcmp(pb,data+16,24));
 assert(!memcmp(pb+128,iosb,sizeof(iosb)));
 /* Reject both buffers if IOSB mapping disappears at physical layer. */
 a[2]=0xd000|7;
 memset(pa,0x77,sizeof(pa));memset(pb,0x77,sizeof(pb));
 assert(port_user_copyv_to(&m,output,2,physical,0)==-1);
 assert(pa[4080]==0x77 && pa[4095]==0x77 && pb[0]==0x77);
 a[2]=0xb000|7;
 /* Reject both if IOSB PTE turns read-only, not even DATA partial writes. */
 a[2]=0xb000|5;
 assert(port_user_copyv_to(&m,output,2,physical,0)==-1);
 assert(pa[4080]==0x77);
 a[2]=0xb000|7;
 /* Distinct virtual ranges that are the SAME physical bytes are rejected. */
 a[3]=0xa000|7;
 output[0]=(struct port_user_output){0x1000,data,16};
 output[1]=(struct port_user_output){0x3000,iosb,12};
 memset(pa,0x77,sizeof(pa));
 assert(port_user_copyv_to(&m,output,2,physical,0)==-1);
 assert(pa[0]==0x77);
 /* Overlapping virtual destinations same page also fail. */
 output[1].destination=0x1008;
 assert(port_user_copyv_to(&m,output,2,physical,0)==-1);
 /* Cross a PDE with a separate page table and physical frame. */
 output[0]=(struct port_user_output){0x3ffff0,data,32};
 output[1]=(struct port_user_output){0x2040,iosb,12};
 assert(port_user_copyv_to(&m,output,2,physical,0)==0);
 assert(!memcmp(pa+4080,data,16));assert(!memcmp(pc,data+16,16));
 assert(!memcmp(pb+64,iosb,12));
 assert(port_user_copyv_to(&m,output,0,physical,0)==-1);
 assert(port_user_copyv_to(&m,output,3,physical,0)==-1);
 output[1].source=0;
 assert(port_user_copyv_to(&m,output,2,physical,0)==-1);
 puts("M34 transactional copyv validation, alias overlap, PDE crossing PASS");
 return 0;
}
