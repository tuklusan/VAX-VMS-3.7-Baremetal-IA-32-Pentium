#include "vms_sysqio_iosb.h"

/* SYSQIOREQ.MAR 286-292: a null IOSB is allowed. Otherwise prove the full
 * 8-byte caller span writable before clearing either longword. */
uint32_t port_v37_sysqio_clear_iosb(uint32_t iosb,
    const struct port_v37_sysqio_memory *memory){
    if(!iosb)return PORT_V37_SYSQIO_IOSB_OK;
    if(!memory||!memory->probe_write||!memory->clear_memory)
        return PORT_V37_SYSQIO_IOSB_BAD;
    if(memory->probe_write(memory->context,iosb,
                           PORT_V37_SYSQIO_IOSB_BYTES)!=0)
        return PORT_V37_SYSQIO_IOSB_ACCVIO;
    if(memory->clear_memory(memory->context,iosb,
                            PORT_V37_SYSQIO_IOSB_BYTES)!=0)
        return PORT_V37_SYSQIO_IOSB_ACCVIO;
    return PORT_V37_SYSQIO_IOSB_OK;
}
