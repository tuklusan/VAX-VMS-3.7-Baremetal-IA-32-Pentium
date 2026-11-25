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
    VMS37_UAF_FLAGS_DISMAIL = 1u << 6,
    VMS37_UAF_PDAYHOURS = 212,
    VMS37_UAF_PDAYFLAGS = 216,
    VMS37_UAF_SDAYHOURS = 217,
    VMS37_UAF_SDAYFLAGS = 221,
    VMS37_UAF_PRIMEDAYS = 224,
    VMS37_UAF_DAY_DISDIALUP = 1u << 0,
    VMS37_UAF_DAY_DISNETWORK = 1u << 1
};
int vms37_uaf_decode(struct port_uaf_record *dst, const uint8_t *bytes,
                     unsigned len);
enum { VMS37_UAF_ACCESS_OK=1, VMS37_UAF_ACCESS_BAD=2,
       VMS37_UAF_ACCESS_HOUR=4, VMS37_UAF_ACCESS_NETWORK=6,
       VMS37_UAF_ACCESS_DIALUP=8 };
/* LOGIN.B32 CHECK_USER_HOURS: weekday is Monday=1 through Sunday=7;
 * explicit_day_flag is EXE$V_EXPLICITP (1 selects secondary-day hours). */
uint32_t vms37_uaf_check_login_access(const struct port_uaf_record *record,
    uint8_t weekday,uint8_t hour,uint8_t explicit_day,uint8_t explicit_day_flag,
    uint8_t remote_terminal,uint8_t dialup_terminal);
#endif
