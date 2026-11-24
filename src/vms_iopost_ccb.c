#include "vms_iopost_end.h"

static uint16_t ccb_read16(const uint8_t *p){
    return (uint16_t)((uint16_t)p[0]|((uint16_t)p[1]<<8));
}
static uint32_t ccb_read32(const uint8_t *p){
    return (uint32_t)p[0]|((uint32_t)p[1]<<8)|
           ((uint32_t)p[2]<<16)|((uint32_t)p[3]<<24);
}
static void ccb_write16(uint8_t *p,uint16_t value){
    p[0]=(uint8_t)value;p[1]=(uint8_t)(value>>8);
}
static void ccb_write32(uint8_t *p,uint32_t value){
    p[0]=(uint8_t)value;p[1]=(uint8_t)(value>>8);
    p[2]=(uint8_t)(value>>16);p[3]=(uint8_t)(value>>24);
}

/* Source DIRPOST sign-extends the IRP's negative channel byte offset, then
 * applies DECW/INCW to CCB$W_IOC. IOC$WAKACP receives the saved DIRP and UCB
 * only when the ordinary I/O decrement makes the channel idle. */
uint32_t port_v37_iopost_ccb_complete(uint8_t *ccb_base,
    size_t preceding_bytes,uint16_t channel_negative,
    const struct port_v37_iopost_ccb_ops *ops,uint16_t *outstanding,
    uint32_t *deaccess_irp){
    if(!ccb_base||!outstanding||!deaccess_irp||
       ((uintptr_t)ccb_base&3u)||channel_negative<UINT16_C(0x8000))
        return PORT_V37_IOPOST_BAD;
    const uint32_t offset=UINT32_C(0x10000)-(uint32_t)channel_negative;
    if(!offset||offset>preceding_bytes||offset%PORT_V37_CCB_BYTES)
        return PORT_V37_IOPOST_BAD;
    uint8_t *const ccb=ccb_base-offset;
    const uint16_t old_count=ccb_read16(ccb+PORT_V37_CCB_IOC);
    const uint16_t new_count=(uint16_t)(old_count-1u);
    const uint32_t pending=old_count==1u?
        ccb_read32(ccb+PORT_V37_CCB_DIRP):0u;
    const uint32_t ucb=pending?ccb_read32(ccb+PORT_V37_CCB_UCB):0u;
    if(pending&&(!ops||!ops->wake_acp||!ucb))
        return PORT_V37_IOPOST_CCB_DEFER;
    ccb_write16(ccb+PORT_V37_CCB_IOC,new_count);
    *outstanding=new_count;*deaccess_irp=0u;
    if(pending){
        ccb_write32(ccb+PORT_V37_CCB_DIRP,0u);
        const uint16_t wake_count=(uint16_t)(new_count+1u);
        ccb_write16(ccb+PORT_V37_CCB_IOC,wake_count);
        *outstanding=wake_count;*deaccess_irp=pending;
        ops->wake_acp(ops->context,pending,ucb);
    }
    return PORT_V37_IOPOST_OK;
}
