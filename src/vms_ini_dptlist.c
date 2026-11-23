#include "vms_ini_dptlist.h"
/* No CPU emulation: 386 absolute 32-bit pointers and source-defined
 * VAX queue topology implemented as direct IA-32 memory writes. */
static uint32_t rd(const uint8_t *p){
    return (uint32_t)p[0]|((uint32_t)p[1]<<8)|
           ((uint32_t)p[2]<<16)|((uint32_t)p[3]<<24);
}
static void wr(uint8_t *p,uint32_t x){
    p[0]=(uint8_t)x;p[1]=(uint8_t)(x>>8);
    p[2]=(uint8_t)(x>>16);p[3]=(uint8_t)(x>>24);
}
static int contained(uint32_t base,size_t cap,uint32_t address){
    return address>=base &&
           (uint64_t)address-(uint64_t)base+8u<=cap &&
           !(address&3u);
}
static uint8_t *node(uint8_t *memory,uint32_t base,uint32_t address){
    return memory+(address-base);
}
uint32_t port_ini_dpt_attach_three(uint8_t *memory,size_t capacity,
    uint32_t guest_memory_base,uint32_t sentinel_address,
    uint32_t mb_driver_address,uint32_t nl_driver_address,
    uint32_t op_driver_address,struct port_ini_dpt_result *out){
    if(!memory||!out || capacity<32u || capacity>UINT32_MAX ||
       !guest_memory_base || (guest_memory_base&3u) ||
       (uintptr_t)memory&3u ||
       (uint64_t)guest_memory_base+capacity>UINT64_C(0x100000000))
        return PORT_INI_DPT_BADPARAM;
    const uint32_t addresses[4]={sentinel_address,mb_driver_address,
                                  nl_driver_address,op_driver_address};
    for(unsigned i=0;i<4;i++){
        if(!contained(guest_memory_base,capacity,addresses[i]))
            return PORT_INI_DPT_BADPARAM;
        for(unsigned j=0;j<i;j++){
            uint32_t d=addresses[i]>addresses[j]?
                       addresses[i]-addresses[j]:addresses[j]-addresses[i];
            if(d<8u)return PORT_INI_DPT_BADPARAM;
        }
    }
    uint8_t *head=node(memory,guest_memory_base,sentinel_address);
    if(rd(head)!=sentinel_address || rd(head+4u)!=sentinel_address)
        return PORT_INI_DPT_NOTEMPTY;
    for(unsigned i=1;i<4;i++){
        uint8_t *p=node(memory,guest_memory_base,addresses[i]);
        if(rd(p)||rd(p+4u))return PORT_INI_DPT_NOTEMPTY;
    }
    /* INSQUE <entry>,IOC$GL_DPTLIST, three times. All pointers are
     * checked before writes, so an invalid caller leaves bytes intact. */
    uint32_t next=sentinel_address;
    for(unsigned i=1;i<4;i++){
        uint8_t *p=node(memory,guest_memory_base,addresses[i]);
        wr(p,next);wr(p+4u,sentinel_address);
        if(next==sentinel_address){
            wr(head+4u,addresses[i]);
        }else{
            wr(node(memory,guest_memory_base,next)+4u,addresses[i]);
        }
        wr(head,addresses[i]);
        next=addresses[i];
    }
    *out=(struct port_ini_dpt_result){sentinel_address,op_driver_address,
             mb_driver_address,3u};
    return PORT_INI_DPT_OK;
}
