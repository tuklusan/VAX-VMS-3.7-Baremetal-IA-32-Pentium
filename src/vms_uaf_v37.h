#ifndef PORT_VMS_UAF_V37_H
#define PORT_VMS_UAF_V37_H
#include <stdint.h>

struct port_uaf_record;
/* SHRLIB/SYSDEF.MDL, UAF structure. The fixed part is 284 bytes.
 * RMS indexed-file metadata is separate from each account record. */
enum {
    VMS37_UAF_FIXED = 284,
    VMS37_UAF_USERNAME = 0,
    VMS37_UAF_HASH = 12,
    VMS37_UAF_ACCOUNT = 20,
    VMS37_UAF_UIC = 28,
    VMS37_UAF_DIRECTORY = 32,
    VMS37_UAF_DEVICE = 64,
    VMS37_UAF_CLI = 80,
    VMS37_UAF_ENCRYPT = 89,
    VMS37_UAF_PRIVILEGES = 94,
    VMS37_UAF_FLAGS = 161,
    VMS37_UAF_SALT = 208,
    VMS37_UAF_FLAGS_CAPTIVE = 1u << 3,
    VMS37_UAF_FLAGS_DISACNT = 1u << 4,
    VMS37_UAF_FLAGS_DISWELCOM = 1u << 5,
    VMS37_UAF_FLAGS_DISMAIL = 1u << 6
};
int vms37_uaf_decode(struct port_uaf_record *dst, const uint8_t *bytes,
                     unsigned len);
#endif
