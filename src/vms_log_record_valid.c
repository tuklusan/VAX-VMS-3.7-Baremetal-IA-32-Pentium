#include "vms_log_system.h"
/* LOGNAMSUB.MAR LOG$SEARCHLOG source record layout from SYSDEF.MDL.
 * Validate the original ASCIC name and actual staged equivalence bounds.
 * Do not infer that any translated device is mounted or has a driver. */
uint32_t port_log_record_valid(const uint8_t *r,size_t n,
                              const char *name,size_t length){
    if(!r||!name||!length||length>PORT_LOG_MAX_NAME||
       n<PORT_LOG_REC_LEN||port_v37_read16(r+8u)!=PORT_LOG_REC_LEN||
       r[10u]!=PORT_V37_DYN_LOG||r[11u]!=PORT_LOG_SYSTEM||
       r[20u]!=length||21u+length+1u>PORT_LOG_REC_LEN||
       r[21u+length]>PORT_LOG_REC_LEN-22u-length||
       r[20u+length+1u]==0u)return PORT_LOG_BAD;
    for(size_t i=0;i<length;i++)if(r[21u+i]!=(uint8_t)name[i])return PORT_LOG_BAD;
    return PORT_LOG_OK;
}
