#include "vms_ini_dpt_templates.h"
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#define CHECK(x) do{if(!(x)){fprintf(stderr,"FAIL %s:%d: %s\n",__FILE__,__LINE__,#x);return 1;}}while(0)
static unsigned rd16(const uint8_t *p){return p[0]|((unsigned)p[1]<<8);}
int main(void){
    uint8_t data[3][PORT_V37_DPT_STAGE_BYTES];
    memset(data,0xa5,sizeof data);
    CHECK(!port_v37_dpt_fill(0,PORT_V37_DPT_STAGE_BYTES,116u,"NLDRIVER",8u));
    CHECK(!port_v37_dpt_fill(data[0],59u,116u,"NLDRIVER",8u));
    CHECK(data[0][0]==0xa5u);
    CHECK(!port_v37_dpt_fill(data[0],60u,116u,"NLDRIVER",12u));
    CHECK(data[0][0]==0xa5u);
    CHECK(port_v37_dpt_stage_mb(data[0]));
    CHECK(port_v37_dpt_stage_nl(data[1]));
    CHECK(port_v37_dpt_stage_op(data[2]));
    CHECK(port_v37_dpt_validate_staged((const uint8_t (*)[60])data));
    const unsigned ucbs[]={132u,116u,188u};
    const char *names[]={"MBDRIVER","NLDRIVER","OPERATOR"};
    for(unsigned i=0;i<3u;i++){
        uint8_t *r=data[i];
        CHECK(r[10]==30u && r[11]==0u && r[12]==1u && r[13]==0u);
        CHECK(rd16(r+8u)==0u); /* deliberate nonpublishable size */
        CHECK(rd16(r+14u)==ucbs[i]);
        CHECK(rd16(r+16u)==56u && rd16(r+18u)==56u);
        CHECK(rd16(r+22u)==8u && rd16(r+24u)==3u && rd16(r+26u)==1u);
        CHECK(r[32]==8u && memcmp(r+33u,names[i],8u)==0);
        for(unsigned j=41u;j<60u;j++)CHECK(r[j]==0u);
    }
    data[2][10u]=0u;
    CHECK(!port_v37_dpt_validate_staged((const uint8_t (*)[60])data));
    data[2][10u]=PORT_V37_DYN_DPT;
    data[1][8u]=0x40u; /* fabricated live image length must be rejected */
    CHECK(!port_v37_dpt_validate_staged((const uint8_t (*)[60])data));
    puts("VMS 3.7 MB/NL/OP source-specific 56-byte DPT prologues, unlinked PASS");
    return 0;
}
