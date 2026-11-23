#include <stdint.h>
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "vms_userio.h"
static uint32_t pd[1024],pt[1024];
static uint8_t a[4096],b[4096],c[4096];
static const uint32_t *table(void *ctx,uint32_t p){(void)ctx;return p==0x5000?pt:0;}
static uint8_t *physical(void *ctx,uint32_t p){
 (void)ctx;return p==0xa000?a:p==0xb000?b:p==0xc000?c:0;
}
int main(void){
 struct port_user_map m={pd,table,0};
 pd[0]=0x5000|7;pt[1]=0xa000|5;pt[2]=0xb000|7;pt[3]=0xc000|7;
 struct port_user_io_desc in={1,0x3000,0x2200},out={7,8,9};
 memcpy(a+4088,&in,8);memcpy(b,((uint8_t *)&in)+8,4);
 assert(port_user_io_decode(&m,0x1ff8,physical,0,&out)==0);
 assert(out.lba==1&&out.data_va==0x3000&&out.iosb_va==0x2200);
 /* Virtual-data/IOSB overlap rejected. */
 in.iosb_va=0x3004;memcpy(b,((uint8_t *)&in)+8,4);
 assert(port_user_io_decode(&m,0x1ff8,physical,0,&out)==-1);
 assert(out.iosb_va==0x2200);
 /* Supervisor output and read-only output rejected. */
 in.iosb_va=0x10000;memcpy(b,((uint8_t *)&in)+8,4);
 assert(port_user_io_decode(&m,0x1ff8,physical,0,&out)==-1);
 in.iosb_va=0x1000;memcpy(b,((uint8_t *)&in)+8,4);
 assert(port_user_io_decode(&m,0x1ff8,physical,0,&out)==-1);
 /* Missing page in descriptor itself must leave output untouched. */
 in.iosb_va=0x2200;memcpy(b,((uint8_t *)&in)+8,4);
 pt[2]=0xd000|7;
 assert(port_user_io_decode(&m,0x1ff8,physical,0,&out)==-1);
 assert(out.iosb_va==0x2200);
 puts("M35 CPL3 read-only descriptor, cross page, malformed outputs PASS");
}
