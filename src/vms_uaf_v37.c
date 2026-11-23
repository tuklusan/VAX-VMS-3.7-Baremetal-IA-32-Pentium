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
