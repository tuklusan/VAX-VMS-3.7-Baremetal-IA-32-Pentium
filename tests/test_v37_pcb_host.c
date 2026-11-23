#include "vms_v37_pcb.h"
#include "vms_v37_dyn_tags.h"
#include <stdio.h>
#include <string.h>
#define CHECK(x) do{if(!(x)){fprintf(stderr,"VMS PCB FAIL %d: %s\n",__LINE__,#x);return 1;}}while(0)
static uint16_t rd16(const uint8_t *p){return (uint16_t)(p[0]|((uint16_t)p[1]<<8));}
static uint32_t rd32(const uint8_t *p){return (uint32_t)p[0]|((uint32_t)p[1]<<8)|((uint32_t)p[2]<<16)|((uint32_t)p[3]<<24);}
int main(void){
    enum {PCB_VA=0x326000u};
    uint8_t bytes[PORT_V37_PCB_BYTES+8u];memset(bytes,0xa5,sizeof bytes);
    struct port_v37_pcb_seed system={.address=PCB_VA,
        .pid=0x00010020u,.priority=16u};
    CHECK(port_v37_pcb_init_genpcb(bytes,sizeof bytes,&system)==PORT_V37_PCB_OK);
    CHECK(rd32(bytes+PORT_V37_PCB_SQFL)==PCB_VA+PORT_V37_PCB_SQFL);
    CHECK(rd32(bytes+PORT_V37_PCB_SQBL)==PCB_VA+PORT_V37_PCB_SQFL);
    CHECK(rd16(bytes+PORT_V37_PCB_SIZE)==PORT_V37_PCB_BYTES);
    CHECK(bytes[PORT_V37_PCB_BTYPE]==PORT_V37_DYN_PCB);
    CHECK(bytes[PORT_V37_PCB_PRI]==15u&&bytes[PORT_V37_PCB_WTIME]==15u&&
          bytes[PORT_V37_PCB_WTIME+1u]==15u&&bytes[PORT_V37_PCB_PRIB]==15u);
    CHECK(bytes[PORT_V37_PCB_ASTEN]==0x0fu);
    CHECK(rd32(bytes+PORT_V37_PCB_PHYPCB)==0u);
    CHECK(rd32(bytes+PORT_V37_PCB_STS)==
          (PORT_V37_PCB_STS_RESIDENT|PORT_V37_PCB_STS_PSWAPM));
    CHECK(rd16(bytes+PORT_V37_PCB_STATE)==PORT_V37_PCB_STATE_CURRENT);
    CHECK(rd16(bytes+PORT_V37_PCB_DIOCNT)==6u&&
          rd16(bytes+PORT_V37_PCB_DIOLM)==6u);
    CHECK(rd32(bytes+PORT_V37_PCB_PID)==system.pid&&
          rd32(bytes+PORT_V37_PCB_PHD)==0u);
    CHECK(rd32(bytes+PORT_V37_PCB_PRIV)==UINT32_MAX&&
          rd32(bytes+PORT_V37_PCB_PRIV+4u)==UINT32_MAX);
    CHECK(rd32(bytes+PORT_V37_PCB_ARB)==PCB_VA+PORT_V37_PCB_ARB&&
          rd32(bytes+PORT_V37_PCB_UIC)==0u);
    CHECK(rd32(bytes+PORT_V37_PCB_ASTQFL)==PCB_VA+PORT_V37_PCB_ASTQFL&&
          rd32(bytes+PORT_V37_PCB_ASTQBL)==PCB_VA+PORT_V37_PCB_ASTQFL);
    CHECK(rd32(bytes+PORT_V37_PCB_LOCKQFL)==PCB_VA+PORT_V37_PCB_LOCKQFL&&
          rd32(bytes+PORT_V37_PCB_LOCKQFL+4u)==PCB_VA+PORT_V37_PCB_LOCKQFL);
    CHECK(bytes[PORT_V37_PCB_BYTES]==0xa5u);

    CHECK(port_v37_pcb_init_syspcb_template(bytes,sizeof bytes,PCB_VA)==
          PORT_V37_PCB_OK&&rd32(bytes+PORT_V37_PCB_PID)==0x00010000u&&
          rd32(bytes+PORT_V37_PCB_PHD)==0u);
    CHECK(port_v37_pcb_finish_syspcb(bytes,sizeof bytes,32u,0x328000u,
          0x328074u)==PORT_V37_PCB_OK&&
          rd32(bytes+PORT_V37_PCB_PID)==0x00010020u&&
          rd32(bytes+PORT_V37_PCB_PHD)==0x328000u&&
          rd32(bytes+PORT_V37_PCB_PHYPCB)==0x328074u);

    const uint8_t name[]="NULL";
    const struct port_v37_pcb_seed nullpcb={.address=PCB_VA,
        .pid=0x00010000u,.priority=0u,.name=(const char *)name,
        .name_bytes=sizeof name-1u};
    CHECK(port_v37_pcb_init_genpcb(bytes,sizeof bytes,&nullpcb)==PORT_V37_PCB_OK);
    CHECK(bytes[PORT_V37_PCB_LNAME]==4u&&
          memcmp(bytes+PORT_V37_PCB_LNAME+1u,name,4u)==0&&
          bytes[PORT_V37_PCB_PRI]==31u&&rd32(bytes+PORT_V37_PCB_PID)==0x10000u);
    CHECK(port_v37_pcb_bind_context(bytes,sizeof bytes,0x328000u,
          0x328074u)==PORT_V37_PCB_OK&&
          rd32(bytes+PORT_V37_PCB_PHD)==0x328000u&&
          rd32(bytes+PORT_V37_PCB_PHYPCB)==0x328074u);
    uint8_t saved[sizeof bytes];memcpy(saved,bytes,sizeof bytes);
    CHECK(port_v37_pcb_bind_context(bytes,sizeof bytes,0x328001u,
          0x328074u)==PORT_V37_PCB_BADPARAM&&
          memcmp(saved,bytes,sizeof bytes)==0);
    struct port_v37_pcb_seed invalid=nullpcb;invalid.address++;
    CHECK(port_v37_pcb_init_genpcb(bytes,sizeof bytes,&invalid)==PORT_V37_PCB_BADPARAM);
    CHECK(memcmp(saved,bytes,sizeof bytes)==0);
    CHECK(port_v37_pcb_init_genpcb(bytes,PORT_V37_PCB_BYTES-1u,&nullpcb)==
          PORT_V37_PCB_BADPARAM&&memcmp(saved,bytes,sizeof bytes)==0);
    puts("SYSDEF.MDL PCB layout and PDAT.MAR GENPCB initialization PASS");
    return 0;
}
