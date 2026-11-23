#include "vms_ini_procvec.h"
#include <stdint.h>
static void store16(uint8_t *p,uint16_t x){p[0]=(uint8_t)x;p[1]=(uint8_t)(x>>8);}
static void store32(uint8_t *p,uint32_t x){
    for(unsigned i=0;i<4u;i++)p[i]=(uint8_t)(x>>(8u*i));
}
static uint32_t load32(const uint8_t *p){
    return (uint32_t)p[0]|((uint32_t)p[1]<<8)|
           ((uint32_t)p[2]<<16)|((uint32_t)p[3]<<24);
}
/* Original source: allocation (MAXPRCCT+1)*6+12, last pointer slot
 * points to the system PCB, all other slots point to NULLPCB, sequence
 * values all zero; max PIX is MAXPRCCT-1, not MAXPRCCT. */
uint32_t port_ini_procvec_init(void *memory,size_t capacity,
                  const struct port_ini_proc_settings *s,
                  struct port_ini_procvec *out){
    if(!memory || !s || !out || ((uintptr_t)memory&3u) ||
       !s->max_processes || s->max_processes>8191u ||
       !s->system_pcb_address || !s->null_pcb_address ||
       (s->system_pcb_address&3u) || (s->null_pcb_address&3u))
        return PORT_INI_PROC_BADPARAM;
    uint32_t count=s->max_processes+1u;
    uint32_t bytes=12u+6u*count;
    if(bytes>UINT16_MAX)return PORT_INI_PROC_BADPARAM;
    if(bytes>capacity)return PORT_INI_PROC_NOMEM;
    uint8_t *pool=(uint8_t *)memory;
    for(uint32_t i=0;i<bytes;i++)pool[i]=0u;
    store16(pool+8u,(uint16_t)bytes);
    store16(pool+10u,s->dyn_pcbvec_type);
    uint8_t *pv=pool+12u;
    uint8_t *sv=pv+4u*count;
    for(uint32_t i=0;i<s->max_processes;i++){
        store32(pv+4u*i,s->null_pcb_address);
        store16(sv+2u*i,0u);
    }
    store32(pv+4u*s->max_processes,s->system_pcb_address);
    store16(sv+2u*s->max_processes,0u);
    *out=(struct port_ini_procvec){pv,sv,s->max_processes,
                  s->max_processes-1u,count,bytes};
    return PORT_INI_PROC_OK;
}
uint32_t port_ini_procvec_ptr(const struct port_ini_procvec *p,uint32_t pix){
    if(!p || !p->pcb_vector || pix>=p->allocated_slots)return 0u;
    return load32(p->pcb_vector+pix*4u);
}
uint32_t port_ini_procvec_set_pcb(struct port_ini_procvec *p,uint32_t pix,
                                  uint32_t pcb_address){
    if(!p||!p->pcb_vector||pix>=p->allocated_slots||!pcb_address||
       (pcb_address&3u))return PORT_INI_PROC_BADPARAM;
    store32(p->pcb_vector+pix*4u,pcb_address);
    return PORT_INI_PROC_OK;
}
uint16_t port_ini_procvec_seq(const struct port_ini_procvec *p,uint32_t pix){
    if(!p || !p->sequence_vector || pix>=p->allocated_slots)return UINT16_MAX;
    const uint8_t *x=p->sequence_vector+pix*2u;
    return (uint16_t)x[0]|(uint16_t)x[1]<<8;
}
