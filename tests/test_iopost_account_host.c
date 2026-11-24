#include "vms_iopost_end.h"
#include <assert.h>
#include <stdint.h>
#include <stdio.h>

static uint32_t read32(const uint8_t *p){
    return (uint32_t)p[0]|((uint32_t)p[1]<<8)|
           ((uint32_t)p[2]<<16)|((uint32_t)p[3]<<24);
}
static void write32(uint8_t *p,uint32_t v){
    p[0]=(uint8_t)v;p[1]=(uint8_t)(v>>8);
    p[2]=(uint8_t)(v>>16);p[3]=(uint8_t)(v>>24);
}
int main(void){
    _Alignas(4) uint8_t phd[PORT_V37_PHD_BYTES]={0};
    write32(phd+PORT_V37_PHD_DIOCNT,7u);
    write32(phd+PORT_V37_PHD_BIOCNT,UINT32_MAX);
    assert(port_v37_iopost_account_phd(phd,sizeof phd,0u)==PORT_V37_IOPOST_OK);
    assert(port_v37_iopost_account_phd(phd,sizeof phd,1u)==PORT_V37_IOPOST_OK);
    assert(read32(phd+PORT_V37_PHD_DIOCNT)==8u&&
           read32(phd+PORT_V37_PHD_BIOCNT)==0u);
    assert(port_v37_iopost_account_phd(phd,PORT_V37_PHD_BIOCNT+3u,0u)==
           PORT_V37_IOPOST_BAD);
    assert(port_v37_iopost_account_phd(phd,sizeof phd,2u)==PORT_V37_IOPOST_BAD);
    assert(read32(phd+PORT_V37_PHD_DIOCNT)==8u&&
           read32(phd+PORT_V37_PHD_BIOCNT)==0u);
    puts("IOCIOPOST DIRPOST PHD direct/buffered I/O accounting PASS");
    return 0;
}
