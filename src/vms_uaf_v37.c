#include "vms_uaf_v37.h"
#include "vms_validate.h"

int vms37_uaf_decode(struct port_uaf_record *dst, const uint8_t *data,
                     unsigned size) {
    if (!dst || !data || size < VMS37_UAF_FIXED)
        return 0;
    for (unsigned i = 0; i < sizeof *dst; i++)
        ((uint8_t *)dst)[i] = 0;
    for (unsigned i = 0; i < PORT_UAF_KEY_BYTES; ++i)
        dst->username[i] = data[VMS37_UAF_USERNAME+i];
    for (unsigned i = 0; i < PORT_UAF_HASH_BYTES; ++i)
        dst->hash[i] = data[VMS37_UAF_HASH+i];
    dst->algorithm = data[VMS37_UAF_ENCRYPT];
    dst->salt_lo = data[VMS37_UAF_SALT];
    dst->salt_hi = data[VMS37_UAF_SALT+1];
    unsigned f = data[VMS37_UAF_FLAGS];
    dst->reserved[0] = (uint8_t)(
        ((f & VMS37_UAF_FLAGS_DISACNT) ? 1u : 0u) |
        ((f & VMS37_UAF_FLAGS_CAPTIVE) ? 2u : 0u) |
        ((f & VMS37_UAF_FLAGS_DISWELCOM) ? 4u : 0u) |
        ((f & VMS37_UAF_FLAGS_DISMAIL) ? 8u : 0u));
    /* Keep all VMS 3.7 fixed record fields for later LOGIN/CLI services. */
    for (unsigned i = 0; i < VMS37_UAF_FIXED; ++i)
        dst->reserved[8+i] = data[i];
    return 1;
}

static uint8_t uaf_byte(const struct port_uaf_record *record,unsigned offset){
    return record->reserved[8u+offset];
}
static uint32_t uaf_long(const struct port_uaf_record *record,unsigned offset){
    return (uint32_t)uaf_byte(record,offset)|
        ((uint32_t)uaf_byte(record,offset+1u)<<8u)|
        ((uint32_t)uaf_byte(record,offset+2u)<<16u)|
        ((uint32_t)uaf_byte(record,offset+3u)<<24u);
}
uint32_t vms37_uaf_check_login_access(const struct port_uaf_record *record,
    uint8_t weekday,uint8_t hour,uint8_t explicit_day,uint8_t explicit_day_flag,
    uint8_t remote_terminal,uint8_t dialup_terminal){
    if(!record||weekday<1u||weekday>7u||hour>23u||explicit_day>1u||
       explicit_day_flag>1u||remote_terminal>1u||dialup_terminal>1u)
        return VMS37_UAF_ACCESS_BAD;
    const int secondary=explicit_day?explicit_day_flag:
        ((uaf_byte(record,VMS37_UAF_PRIMEDAYS)&(1u<<(weekday-1u)))!=0u);
    const int primary=!secondary;
    const unsigned hours_offset=primary?VMS37_UAF_PDAYHOURS:
                                         VMS37_UAF_SDAYHOURS;
    const unsigned flags_offset=primary?VMS37_UAF_PDAYFLAGS:
                                         VMS37_UAF_SDAYFLAGS;
    /* LOGIN.B32 signals LGI$_BADHOUR when the selected hour bit is set. */
    if(uaf_long(record,hours_offset)&(UINT32_C(1)<<hour))
        return VMS37_UAF_ACCESS_HOUR;
    const uint8_t flags=uaf_byte(record,flags_offset);
    if(remote_terminal&&(flags&VMS37_UAF_DAY_DISNETWORK))
        return VMS37_UAF_ACCESS_NETWORK;
    if(dialup_terminal&&(flags&VMS37_UAF_DAY_DISDIALUP))
        return VMS37_UAF_ACCESS_DIALUP;
    return VMS37_UAF_ACCESS_OK;
}
