#include <stdint.h>
#include <assert.h>
#include <string.h>
#include <stdio.h>
#include "vms_userptr.h"
#include "vms_exe_access.h"
static uint32_t pd[1024],pt0[1024],pt1[1024];
static uint8_t p0[4096],p1[4096];
static const uint32_t *lookup(void *ctx,uint32_t p){
 (void)ctx;return p==0x5000?pt0:p==0x6000?pt1:0;
}
static uint8_t *phys(void *ctx,uint32_t p){
 (void)ctx;return p==0xa000?p0:p==0x9000?p1:0;
}
int main(void){
 struct port_user_map m={pd,lookup,0};
 pd[0]=0x5000|7;pt0[1]=0xa000|7;pt0[2]=0x9000|5;
 for(unsigned i=0;i<4096;i++){p0[i]=(uint8_t)i;p1[i]=(uint8_t)(i+17);}
 uint8_t buf[80];memset(buf,0x77,sizeof(buf));
 assert(port_user_copy_from(&m,buf,0x1ff0,32,phys,0)==0);
 assert(!memcmp(buf,p0+4080,16));assert(!memcmp(buf+16,p1,16));
 assert(buf[32]==0x77);
 struct port_exe_access exe;struct port_exe_user_access binding;
 port_exe_user_access_init(&exe,&binding,&m,phys,0);
 assert(port_exe_probe_read(&exe,0x1ff0,32));
 memset(buf,0,sizeof buf);assert(port_exe_read(&exe,0x1ff0,buf,32));
 assert(!memcmp(buf,p0+4080,16)&&!memcmp(buf+16,p1,16));
 pt0[2]=0x9000|7;
 assert(port_exe_probe_write(&exe,0x1ff0,32));
 memset(buf,0x5a,32);assert(port_exe_write(&exe,0x1ff0,buf,32));
 assert(p0[4080]==0x5a&&p1[15]==0x5a);
 pt0[2]=0x9000|5;
 assert(!port_exe_probe_write(&exe,0x2000,32));
 /* Read-only page can be copied in, not written to. */
 memset(buf,0x77,sizeof(buf));
 assert(port_user_copy_from(&m,buf,0x2000,32,phys,0)==0);
 assert(!memcmp(buf,p1,32));
 assert(port_user_copy_to(&m,0x2000,buf,32,phys,0)==-1);
 /* Missing physical alias or PTE: no partial destination mutation. */
 pt0[2]=0xb000|5;memset(buf,0x77,sizeof(buf));
 assert(port_user_copy_from(&m,buf,0x1ff0,32,phys,0)==-1);
 for(unsigned i=0;i<32;i++)assert(buf[i]==0x77);
 pt0[2]=0x9000|5;pt0[1]=0xa000|3;
 assert(port_user_copy_from(&m,buf,0x1ff0,32,phys,0)==-1);
 pt0[1]=0xa000|7;
 assert(port_user_copy_from(&m,buf,0,4,phys,0)==-1);
 assert(port_user_copy_from(&m,buf,0xfffffff0u,32,phys,0)==-1);
 assert(port_user_copy_from(&m,0,0x1000,4,phys,0)==-1);
 assert(port_user_copy_from(&m,buf,0x1000,0,phys,0)==-1);
 /* Cross a directory boundary with independent physical pages. */
 pd[1]=0x6000|7;pt0[1023]=0xa000|7;pt1[0]=0x9000|5;
 assert(port_user_copy_from(&m,buf,0x3ffff0,32,phys,0)==0);
 assert(!memcmp(buf,p0+4080,16));assert(!memcmp(buf+16,p1,16));
 puts("M33 copy-from alias, read-only, PDE-crossing, atomic failure PASS");
 return 0;
}
