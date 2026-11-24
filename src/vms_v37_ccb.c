#include "vms_v37_ccb.h"

static uint16_t rd16(const uint8_t *p){
    return (uint16_t)((uint16_t)p[0]|((uint16_t)p[1]<<8));
}
static uint32_t rd32(const uint8_t *p){
    return (uint32_t)p[0]|((uint32_t)p[1]<<8)|
           ((uint32_t)p[2]<<16)|((uint32_t)p[3]<<24);
}

/* SYS/SRC/SYSQIOREQ.MAR VCHAN: retain only channel bits 4..15, reject zero
 * and offsets past CTL$GW_CHINDX, then address CTL$GL_CCBBASE-channel.
 * The AMOD comparison is strict: equal or less privileged callers fail. */
uint32_t port_v37_ccb_resolve(uint8_t *base,size_t preceding_bytes,
    uint32_t channel,uint16_t maximum,uint8_t caller_mode,
    struct port_v37_ccb_view *view){
    if(!base||!view||caller_mode>3u||maximum==0u||
       ((uintptr_t)base&3u))return PORT_V37_CCB_BAD;
    const uint32_t offset=channel&UINT32_C(0x0000fff0);
    if((channel&UINT32_C(0xffff000f))||offset==0u||
       offset>maximum||offset>preceding_bytes||
       (offset%PORT_V37_CCB_BYTES)!=0u)
        return PORT_V37_CCB_IVCHAN;
    uint8_t *const ccb=base-offset;
    const uint32_t ucb=rd32(ccb+PORT_V37_CCB_UCB);
    const uint8_t limit=ccb[PORT_V37_CCB_AMOD];
    if(!ucb||limit==0u||limit>4u)return PORT_V37_CCB_IVCHAN;
    if(caller_mode>=limit)return PORT_V37_CCB_NOPRIV;
    const uint32_t window=rd32(ccb+PORT_V37_CCB_WIND);
    const struct port_v37_ccb_view out={
        .ccb=ccb,.ucb=ucb,.window=window,
        .outstanding=rd16(ccb+PORT_V37_CCB_IOC),
        .status=ccb[PORT_V37_CCB_STS],.access_limit=limit,
        .pending=(uint8_t)((window&PORT_V37_CCB_PENDING)!=0u)};
    *view=out;
    return PORT_V37_CCB_OK;
}
