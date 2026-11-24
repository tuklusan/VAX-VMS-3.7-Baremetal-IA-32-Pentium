#include "vms_iopost_end.h"

_Static_assert(PORT_V37_PHD_BIOCNT==PORT_V37_PHD_DIOCNT+4u,
    "SYSDEFMP.MDL direct and buffered I/O counters must be adjacent");

static uint32_t read32(const uint8_t *p){
    return (uint32_t)p[0]|((uint32_t)p[1]<<8)|
           ((uint32_t)p[2]<<16)|((uint32_t)p[3]<<24);
}
static void write32(uint8_t *p,uint32_t v){
    p[0]=(uint8_t)v;p[1]=(uint8_t)(v>>8);
    p[2]=(uint8_t)(v>>16);p[3]=(uint8_t)(v>>24);
}

/* IOCIOPOST.MAR DIRPOST EXTZV/INCL. The caller must supply the current
 * process PHD selected from SCH$GL_CURPCB; this routine never resolves PID. */
uint32_t port_v37_iopost_account_phd(uint8_t *phd,size_t bytes,
    uint8_t buffered){
    if(!phd||bytes<PORT_V37_PHD_BIOCNT+sizeof(uint32_t)||buffered>1u||
       ((uintptr_t)phd&3u))return PORT_V37_IOPOST_BAD;
    const size_t offset=buffered?PORT_V37_PHD_BIOCNT:PORT_V37_PHD_DIOCNT;
    write32(phd+offset,read32(phd+offset)+1u);
    return PORT_V37_IOPOST_OK;
}
