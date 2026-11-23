#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include "vms_dq_crb.h"
#define CHECK(x) do {if(!(x)){fprintf(stderr,"DQ CRB failed line %u: %s\n",(unsigned)__LINE__,#x);return 1;}}while(0)
static unsigned calls;
static void handler(void){calls++;}
int main(void){
    struct port_dq_crb crb;
    memset(&crb,0xa5,sizeof crb);
    CHECK(port_dq_crb_init(&crb,handler,PORT_DQ_CRB_IRQ14_VECTOR)==PORT_DQ_CRB_OK);
    CHECK(port_dq_crb_validate(&crb)==PORT_DQ_CRB_OK);
    CHECK(crb.vector==46u&&crb.interrupts==0u&&crb.ready==1u);
    for(unsigned i=15u;i<PORT_DQ_CRB_BYTES;i++)CHECK(crb.source[i]==0u);
    CHECK(port_dq_crb_interrupt(&crb,45u)==PORT_DQ_CRB_BAD);
    CHECK(calls==0u&&crb.interrupts==0u);
    CHECK(port_dq_crb_interrupt(&crb,46u)==PORT_DQ_CRB_OK);
    CHECK(calls==1u&&crb.interrupts==1u);
    crb.source[32u]=1u;
    CHECK(port_dq_crb_validate(&crb)==PORT_DQ_CRB_BAD);
    CHECK(port_dq_crb_init(0,handler,46u)==PORT_DQ_CRB_BAD);
    CHECK(port_dq_crb_init(&crb,0,46u)==PORT_DQ_CRB_BAD);
    CHECK(port_dq_crb_init(&crb,handler,45u)==PORT_DQ_CRB_BAD);
    puts("DQ CRB native vector and source-layout checks passed");
    return 0;
}
