#include "vms_v37_pcb.h"
#include "vms_v37_dyn_tags.h"

_Static_assert(PORT_V37_PCB_IPAST+4u==PORT_V37_PCB_BYTES,
               "SYSDEF.MDL PCB fixed length");
_Static_assert(PORT_V37_PCB_PRIV+8u==PORT_V37_PCB_ARB,
               "PCB privilege/ARB adjacency");

static void put16(uint8_t *p,uint16_t x){
    p[0]=(uint8_t)x;p[1]=(uint8_t)(x>>8);
}
static uint16_t get16(const uint8_t *p){
    return (uint16_t)p[0]|(uint16_t)((uint16_t)p[1]<<8);
}
static uint32_t get32(const uint8_t *p){
    return (uint32_t)p[0]|((uint32_t)p[1]<<8)|
        ((uint32_t)p[2]<<16)|((uint32_t)p[3]<<24);
}
static void put32(uint8_t *p,uint32_t x){
    p[0]=(uint8_t)x;p[1]=(uint8_t)(x>>8);
    p[2]=(uint8_t)(x>>16);p[3]=(uint8_t)(x>>24);
}
static void put64(uint8_t *p,uint64_t x){
    put32(p,(uint32_t)x);put32(p+4u,(uint32_t)(x>>32));
}

/* PDAT.MAR GENPCB: construct the fixed PCB source record in source order.
 * The x86 PHD supplies the translated PHYPCB address; this field is never
 * synthesized with the VAX 0x80000000 address bias. */
uint32_t port_v37_pcb_init_genpcb(uint8_t *record,size_t capacity,
    const struct port_v37_pcb_seed *s){
    if(!record||!s||capacity<PORT_V37_PCB_BYTES||!s->address||
       (s->address&7u)||s->address>UINT32_MAX-PORT_V37_PCB_BYTES||
       s->priority>31u||!s->pid||s->name_bytes>15u||
       (s->name_bytes&&!s->name))return PORT_V37_PCB_BADPARAM;
    uint8_t candidate[PORT_V37_PCB_BYTES]={0};
    put32(candidate+PORT_V37_PCB_SQFL,s->address+PORT_V37_PCB_SQFL);
    put32(candidate+PORT_V37_PCB_SQBL,s->address+PORT_V37_PCB_SQFL);
    put16(candidate+PORT_V37_PCB_SIZE,PORT_V37_PCB_BYTES);
    candidate[PORT_V37_PCB_BTYPE]=PORT_V37_DYN_PCB;
    const uint8_t inverse_priority=(uint8_t)(31u-s->priority);
    candidate[PORT_V37_PCB_PRI]=inverse_priority;
    candidate[PORT_V37_PCB_ASTEN]=0x0fu;
    put32(candidate+PORT_V37_PCB_ASTQFL,s->address+PORT_V37_PCB_ASTQFL);
    put32(candidate+PORT_V37_PCB_ASTQBL,s->address+PORT_V37_PCB_ASTQFL);
    put32(candidate+PORT_V37_PCB_PHYPCB,s->phy_pcb_address);
    put32(candidate+PORT_V37_PCB_STS,PORT_V37_PCB_STS_RESIDENT|
        PORT_V37_PCB_STS_PSWAPM);
    candidate[PORT_V37_PCB_WTIME]=inverse_priority; /* PCB$B_PRISAV */
    candidate[PORT_V37_PCB_WTIME+1u]=inverse_priority; /* PCB$B_PRIBSAV */
    put16(candidate+PORT_V37_PCB_STATE,PORT_V37_PCB_STATE_CURRENT);
    candidate[PORT_V37_PCB_PRIB]=inverse_priority;
    put16(candidate+PORT_V37_PCB_DIOCNT,6u);
    put16(candidate+PORT_V37_PCB_DIOLM,6u);
    put32(candidate+PORT_V37_PCB_PID,s->pid);
    put32(candidate+PORT_V37_PCB_PHD,s->phd_address);
    put64(candidate+PORT_V37_PCB_PRIV,UINT64_MAX);
    put32(candidate+PORT_V37_PCB_ARB,s->address+PORT_V37_PCB_ARB);
    put32(candidate+PORT_V37_PCB_UIC,s->uic);
    candidate[PORT_V37_PCB_LNAME]=(uint8_t)s->name_bytes;
    for(size_t i=0;i<s->name_bytes;i++)
        candidate[PORT_V37_PCB_LNAME+1u+i]=(uint8_t)s->name[i];
    put32(candidate+PORT_V37_PCB_LOCKQFL,s->address+PORT_V37_PCB_LOCKQFL);
    put32(candidate+PORT_V37_PCB_LOCKQFL+4u,s->address+PORT_V37_PCB_LOCKQFL);
    for(size_t i=0;i<sizeof candidate;i++)record[i]=candidate[i];
    return PORT_V37_PCB_OK;
}

uint32_t port_v37_pcb_init_syspcb_template(uint8_t *record,size_t capacity,
    uint32_t address){
    const struct port_v37_pcb_seed system={
        .address=address,.pid=UINT32_C(0x00010000),.priority=16u};
    return port_v37_pcb_init_genpcb(record,capacity,&system);
}

uint32_t port_v37_pcb_set_pix(uint8_t *record,size_t capacity,uint16_t pix){
    if(!record||capacity<PORT_V37_PCB_BYTES||
       get16(record+PORT_V37_PCB_SIZE)!=PORT_V37_PCB_BYTES||
       record[PORT_V37_PCB_BTYPE]!=PORT_V37_DYN_PCB||
       (get32(record+PORT_V37_PCB_PID)>>16)!=1u)
        return PORT_V37_PCB_BADPARAM;
    put16(record+PORT_V37_PCB_PID,pix);
    return PORT_V37_PCB_OK;
}

uint32_t port_v37_pcb_finish_syspcb(uint8_t *record,size_t capacity,
    uint16_t system_pix,uint32_t phd_address,uint32_t native_phy_pcb_address){
    if(!record||capacity<PORT_V37_PCB_BYTES||!phd_address||
       !native_phy_pcb_address||
       get16(record+PORT_V37_PCB_SIZE)!=PORT_V37_PCB_BYTES||
       record[PORT_V37_PCB_BTYPE]!=PORT_V37_DYN_PCB||
       (get32(record+PORT_V37_PCB_PID)>>16)!=1u)
        return PORT_V37_PCB_BADPARAM;
    /* INIT.MAR stores a word to the low half of PID, preserving sequence. */
    put16(record+PORT_V37_PCB_PID,system_pix);
    put32(record+PORT_V37_PCB_PHD,phd_address);
    put32(record+PORT_V37_PCB_PHYPCB,native_phy_pcb_address);
    return PORT_V37_PCB_OK;
}
