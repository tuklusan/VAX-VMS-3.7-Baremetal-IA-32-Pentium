#include "vms_sysqio_iosb.h"
#include "vms_sysqio_args.h"

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

/* Async completion writes the eight-byte SYS$QIO status block after the
 * caller may have been descheduled. Use retained physical pages, never the
 * stale user VA under whichever CR3 happens to be current. */
uint32_t port_v37_sysqio_copy_iosb_to_user(
    const struct port_v37_sysqio_prepared *request,
    const struct port_v37_sysqio_iosb_value *value){
    if(!request||!value)return PORT_V37_SYSQIO_BUFFER_BAD;
    if(!request->args.iosb)return PORT_V37_SYSQIO_BUFFER_NONE;
    if(!request->iosb_pages.count)return PORT_V37_SYSQIO_BUFFER_ACCVIO;
    uint8_t bytes[PORT_V37_SYSQIO_IOSB_BYTES]={
        (uint8_t)value->status,(uint8_t)(value->status>>8),
        (uint8_t)value->information,(uint8_t)(value->information>>8),
        (uint8_t)value->device_status,(uint8_t)(value->device_status>>8),
        (uint8_t)(value->device_status>>16),
        (uint8_t)(value->device_status>>24)};
    return port_user_copy_to_pinned(&request->iosb_pages,
        request->args.iosb,bytes,sizeof bytes)==PORT_USER_OK?
        PORT_V37_SYSQIO_BUFFER_OK:PORT_V37_SYSQIO_BUFFER_ACCVIO;
}
