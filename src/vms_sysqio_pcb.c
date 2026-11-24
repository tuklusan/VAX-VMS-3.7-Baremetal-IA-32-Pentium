#include "vms_sysqio_args.h"
#include "vms_v37_pcb.h"
#include "vms_v37_dyn_tags.h"

static uint16_t load16(const uint8_t *p){
    return (uint16_t)((uint16_t)p[0]|((uint16_t)p[1]<<8));
}
static uint32_t load32(const uint8_t *p){
    return (uint32_t)p[0]|((uint32_t)p[1]<<8)|
           ((uint32_t)p[2]<<16)|((uint32_t)p[3]<<24);
}
static void store16(uint8_t *p,int16_t value){
    const uint16_t v=(uint16_t)value;
    p[0]=(uint8_t)v;p[1]=(uint8_t)(v>>8);
}
static int16_t signed16(uint16_t v){
    return (int16_t)(v&0x8000u?(int32_t)v-0x10000:(int32_t)v);
}

/* SYSDEF.MDL PCB quotas, PID and local event flag clusters. The PCB byte
 * record is the owner of live process state; the compact process fields are
 * retained only for isolated host fixtures. */
uint32_t port_v37_sysqio_process_bind_pcb(uint8_t *pcb,size_t bytes,
    uint8_t access_mode,struct port_v37_sysqio_process *process){
    if(!pcb||!process||bytes<PORT_V37_PCB_BYTES||access_mode>3u||
       ((uintptr_t)pcb&3u)||load16(pcb+PORT_V37_PCB_SIZE)!=PORT_V37_PCB_BYTES||
       pcb[PORT_V37_PCB_BTYPE]!=PORT_V37_DYN_PCB||
       !load32(pcb+PORT_V37_PCB_PID))
        return PORT_V37_SYSQIO_PCB_BAD;
    const struct port_v37_sysqio_process out={
        .pid=load32(pcb+PORT_V37_PCB_PID),
        .arb_va=load32(pcb+PORT_V37_PCB_ARB),.access_mode=access_mode,
        .buffered_io_count=signed16(load16(pcb+PORT_V37_PCB_BIOCNT)),
        .direct_io_count=signed16(load16(pcb+PORT_V37_PCB_DIOCNT)),
        .ast_count=signed16(load16(pcb+PORT_V37_PCB_ASTCNT)),
        .source_pcb=pcb,.source_pcb_bytes=bytes};
    *process=out;
    return PORT_V37_SYSQIO_PCB_OK;
}

static int16_t pcb_count(const uint8_t *p){
    return signed16(load16(p));
}
int16_t port_v37_sysqio_process_io_count(
    const struct port_v37_sysqio_process *p,uint8_t buffered){
    if(p&&p->source_pcb&&p->source_pcb_bytes>=PORT_V37_PCB_BYTES)
        return pcb_count(p->source_pcb+(buffered?PORT_V37_PCB_BIOCNT:
                                                   PORT_V37_PCB_DIOCNT));
    return p?(buffered?p->buffered_io_count:p->direct_io_count):0;
}
void port_v37_sysqio_process_set_io_count(
    struct port_v37_sysqio_process *p,uint8_t buffered,int16_t value){
    if(!p)return;
    if(p->source_pcb&&p->source_pcb_bytes>=PORT_V37_PCB_BYTES){
        store16(p->source_pcb+(buffered?PORT_V37_PCB_BIOCNT:
                                         PORT_V37_PCB_DIOCNT),value);
        if(buffered)p->buffered_io_count=value;else p->direct_io_count=value;
    }else if(buffered)p->buffered_io_count=value;
    else p->direct_io_count=value;
}
int16_t port_v37_sysqio_process_ast_count(
    const struct port_v37_sysqio_process *p){
    if(p&&p->source_pcb&&p->source_pcb_bytes>=PORT_V37_PCB_BYTES)
        return pcb_count(p->source_pcb+PORT_V37_PCB_ASTCNT);
    return p?p->ast_count:0;
}
void port_v37_sysqio_process_set_ast_count(
    struct port_v37_sysqio_process *p,int16_t value){
    if(!p)return;
    if(p->source_pcb&&p->source_pcb_bytes>=PORT_V37_PCB_BYTES){
        store16(p->source_pcb+PORT_V37_PCB_ASTCNT,value);
        p->ast_count=value;
    }else p->ast_count=value;
}
