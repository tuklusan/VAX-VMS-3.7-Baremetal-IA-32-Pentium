#include "vms_f11_bootpath.h"
/* The source directory FID search takes the name and version separately.
 * Reject path control characters instead of silently traversing another
 * directory or weakening a Files-11 access boundary. */
uint32_t port_f11_boot_validate_name(const char *s,size_t n){
    if(!s||!n||n>39u)return PORT_F11_PATH_BAD;
    for(size_t i=0;i<n;i++){
        const unsigned c=(unsigned char)s[i];
        if(!((c>='A'&&c<='Z')||(c>='a'&&c<='z')||
             (c>='0'&&c<='9')||c=='_'||c=='-'||c=='.'||c=='$'))
            return PORT_F11_PATH_BAD;
    }
    return PORT_F11_PATH_OK;
}
