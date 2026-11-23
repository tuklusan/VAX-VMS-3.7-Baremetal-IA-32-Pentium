#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "vms_boo_t58bootio.h"
struct rig {int requests;};
static uint32_t physical(void *v,uint8_t unit,uint32_t lbn,uint8_t block[512]){
 struct rig *r=v;r->requests++;assert(unit==0&&lbn==42);memset(block,0x3c,512);return 1;
}
int main(void){struct rig r={0};struct port_t58bootio b={&r,physical};uint8_t block[512]={0};
 assert(port_t58_read_lbn(&b,42,block)==1);assert(r.requests==1&&block[0]==0x3c&&block[511]==0x3c);
 assert(port_t58_read_lbn(NULL,42,block)==PORT_SS_BADPARAM);
 b.read_block=NULL;assert(port_t58_read_lbn(&b,42,block)==PORT_SS_BADPARAM);
 puts("PASS: TU58 boot block callback requires real physical device driver");}
