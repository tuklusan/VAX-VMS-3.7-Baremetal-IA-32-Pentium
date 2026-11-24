#include "vms_iopost_end.h"
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

struct rig {
    uint8_t pcb[PORT_V37_PCB_BYTES];
    uint32_t unlock_calls,pte,pages,kast_calls,pid,pcb_va,irp_va;
};
static uint32_t read16(const uint8_t *p){return (uint32_t)p[0]|((uint32_t)p[1]<<8);}
static void write16(uint8_t *p,uint16_t v){p[0]=(uint8_t)v;p[1]=(uint8_t)(v>>8);}
static void write32(uint8_t *p,uint32_t v){
    p[0]=(uint8_t)v;p[1]=(uint8_t)(v>>8);
    p[2]=(uint8_t)(v>>16);p[3]=(uint8_t)(v>>24);
}
static int pcb_resolve(void *ctx,uint32_t va,uint8_t **out,size_t *bytes){
    struct rig *r=ctx;if(va!=0x2000u)return -1;
    *out=r->pcb;*bytes=sizeof r->pcb;return 0;
}
static void unlock(void *ctx,uint32_t pte,uint32_t pages){
    struct rig *r=ctx;++r->unlock_calls;r->pte=pte;r->pages=pages;
}
static void queue_kast(void *ctx,uint32_t pid,uint32_t pcb,uint32_t irp){
    struct rig *r=ctx;++r->kast_calls;r->pid=pid;r->pcb_va=pcb;r->irp_va=irp;
}
int main(void){
    struct rig r={0};
    const struct port_v37_pcb_seed seed={
        .address=0x2000u,.pid=0x00070002u,.priority=16u,
        .name="IO",.name_bytes=2u};
    assert(port_v37_pcb_init_genpcb(r.pcb,sizeof r.pcb,&seed)==PORT_V37_PCB_OK);
    uint8_t proc_pool[64]={0};struct port_ini_procvec pv={0};
    const struct port_ini_proc_settings ps={
        .max_processes=4u,.system_pcb_address=0x3000u,
        .null_pcb_address=0x1000u,.dyn_pcbvec_type=1u};
    assert(port_ini_procvec_init(proc_pool,sizeof proc_pool,&ps,&pv)==PORT_INI_PROC_OK);
    assert(port_ini_procvec_set_pcb(&pv,2u,0x2000u)==PORT_INI_PROC_OK);
    const struct port_v37_iopost_process_ops ops={
        .context=&r,.process_vector=&pv,.resolve_pcb=pcb_resolve,
        .unlock_pages=unlock,.queue_kast=queue_kast};
    _Alignas(4) uint8_t irp[PORT_V37_IRP_SIZE]={0};
    write32(irp+12u,seed.pid);write32(irp+44u,0x5000u);
    write16(irp+48u,511u);write32(irp+52u,2u);
    write16(r.pcb+PORT_V37_PCB_DIOCNT,UINT16_MAX);
    assert(port_v37_iopost_process_direct(irp,sizeof irp,0x6000u,&ops)==
           PORT_V37_IOPOST_OK);
    assert(read16(r.pcb+PORT_V37_PCB_DIOCNT)==0u&&r.unlock_calls==1u&&
           r.pte==0x5000u&&r.pages==2u&&r.kast_calls==1u&&
           r.pid==seed.pid&&r.pcb_va==0x2000u&&r.irp_va==0x6000u);
    write16(irp+42u,1u);
    assert(port_v37_iopost_process_direct(irp,sizeof irp,0x6000u,&ops)==
           PORT_V37_IOPOST_PROCESS_DEFER);
    assert(read16(r.pcb+PORT_V37_PCB_DIOCNT)==0u&&r.unlock_calls==1u&&
           r.kast_calls==1u);
    write16(irp+42u,UINT16_C(1u<<4));
    assert(port_v37_iopost_process_direct(irp,sizeof irp,0x6000u,&ops)==
           PORT_V37_IOPOST_PROCESS_DEFER);
    assert(read16(r.pcb+PORT_V37_PCB_DIOCNT)==0u&&r.unlock_calls==1u&&
           r.kast_calls==1u);
    write16(irp+42u,0u);write32(irp+12u,0x00080002u);
    assert(port_v37_iopost_process_direct(irp,sizeof irp,0x6000u,&ops)==
           PORT_V37_IOPOST_PROCESS_DEFER);
    assert(read16(r.pcb+PORT_V37_PCB_DIOCNT)==0u&&r.unlock_calls==1u&&
           r.kast_calls==1u);
    puts("IOCIOPOST positive-PID direct completion route and source PCB count PASS");
    return 0;
}
