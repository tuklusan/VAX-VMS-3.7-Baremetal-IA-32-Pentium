#include "vms_ini_dptlist.h"
#include <stdio.h>
#include <string.h>
#include <stdint.h>
#define CHECK(x) do{if(!(x)){fprintf(stderr,"FAIL %s:%d %s\n",__FILE__,__LINE__,#x);return 1;}}while(0)
static uint32_t r32(const uint8_t *p){
    return (uint32_t)p[0]|(uint32_t)p[1]<<8|
           (uint32_t)p[2]<<16|(uint32_t)p[3]<<24;
}
static void w32(uint8_t *p,uint32_t x){
    for(unsigned i=0;i<4;i++)p[i]=(uint8_t)(x>>(8u*i));
}
int main(void){
    uint8_t buffer[160] __attribute__((aligned(8)));
    memset(buffer,0xa5,sizeof buffer);
    /* Four independent original queue header blocks in 32-bit guest RAM. */
    const uint32_t base=0x320000u,head=base+16u,
                   mb=base+40u,nl=base+72u,op=base+104u;
    memset(buffer+16,0,8);memset(buffer+40,0,8);
    memset(buffer+72,0,8);memset(buffer+104,0,8);
    w32(buffer+16,head);w32(buffer+20,head);
    uint8_t before[160];memcpy(before,buffer,sizeof before);
    struct port_ini_dpt_result out={0};
    CHECK(port_ini_dpt_attach_three(buffer,sizeof buffer,base,head,
          mb,nl,op,&out)==PORT_INI_DPT_OK);
    CHECK(out.driver_count==3u&&out.head_address==head);
    CHECK(out.first_driver==op&&out.last_driver==mb);
    /* VAX INSQUE inserts after the head, so OP, NL, MB, head. */
    CHECK(r32(buffer+16)==op&&r32(buffer+20)==mb);
    CHECK(r32(buffer+104)==nl&&r32(buffer+108)==head);
    CHECK(r32(buffer+72)==mb&&r32(buffer+76)==op);
    CHECK(r32(buffer+40)==head&&r32(buffer+44)==nl);
    CHECK(buffer[15]==0xa5&&buffer[24]==0xa5&&buffer[39]==0xa5);
    CHECK(buffer[48]==0xa5&&buffer[80]==0xa5&&buffer[112]==0xa5);
    CHECK(port_ini_dpt_attach_three(buffer,sizeof buffer,base,head,
          mb,nl,op,&out)==PORT_INI_DPT_NOTEMPTY);
    CHECK(port_ini_dpt_attach_three(buffer,sizeof buffer,base,head,
          mb,nl,op,0)==PORT_INI_DPT_BADPARAM);
    CHECK(port_ini_dpt_attach_three(buffer,sizeof buffer,base,head,
          mb,nl,mb,&out)==PORT_INI_DPT_BADPARAM);
    CHECK(port_ini_dpt_attach_three(buffer,sizeof buffer,base,head,
          mb,nl,base+158u,&out)==PORT_INI_DPT_BADPARAM);
    CHECK(port_ini_dpt_attach_three(buffer,sizeof buffer,base,head,
          mb,nl,head+4u,&out)==PORT_INI_DPT_BADPARAM);
    /* Fail-closed before memory mutation, even if the first two entries
     * are structurally valid but the final entry is not. */
    memcpy(buffer,before,sizeof buffer);
    CHECK(port_ini_dpt_attach_three(buffer,sizeof buffer,base,head,
          mb,nl,base+158u,&out)==PORT_INI_DPT_BADPARAM);
    CHECK(!memcmp(buffer,before,sizeof buffer));
    w32(buffer+40,0x1234u);
    CHECK(port_ini_dpt_attach_three(buffer,sizeof buffer,base,head,
          mb,nl,op,&out)==PORT_INI_DPT_NOTEMPTY);
    CHECK(r32(buffer+16)==head&&r32(buffer+20)==head);
    puts("INIT.MAR/DEVICEDAT.MAR source-ordered MB NL OP DPT INSQUE layout PASS");
    return 0;
}
