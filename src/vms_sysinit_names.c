#include "vms_sysinit_names.h"

static const char *const names[PORT_SIP_NAME_COUNT] = {
    "SYS$SYSTEM", "SYS$SHARE", "SYS$MESSAGE"
};
static const char *const paths[PORT_SIP_NAME_COUNT] = {
    "SYSEXE", "SYSLIB", "SYSMSG"
};

uint32_t port_sip_create_system_names(const struct port_sip_logical_ops *ops,
                                       void *context,
                                       const uint8_t top_system[10]) {
    if (!ops || !ops->translate || !ops->create_system || !top_system)
        return PORT_SIP_BADARG;
    char translation[PORT_SIP_NAME_MAX + 1u] = {0};
    unsigned device_length = 0;
    uint32_t status = ops->translate(context,"SYS$SYSDEVICE",translation,
                                    PORT_SIP_NAME_MAX,&device_length);
    if (!(status & 1u)) return status;
    if (!device_length || device_length > PORT_SIP_NAME_MAX)
        return PORT_SIP_TOOLONG;
    /* TRNLOG can return a counted device name. The original strips
     * a leading underscore when forming each file specification. */
    unsigned start = translation[0] == '_' ? 1u : 0u;
    if (start == device_length) return PORT_SIP_BADARG;
    unsigned n = device_length - start;
    for (unsigned i=0; i<n; i++) translation[i]=translation[i+start];
    unsigned top_length = top_system[0];
    if (top_length > 9u) return PORT_SIP_BADARG;
    for (unsigned i=0;i<top_length;i++) {
        char c=(char)top_system[i+1u];
        if (c=='[' || c==']' || c==':' || c=='/' || !c)
            return PORT_SIP_BADARG;
    }
    /* One result buffer for each original table pair.  Use the same
     * base device path for all three names, as the VMS loop does. */
    for (unsigned item=0;item<PORT_SIP_NAME_COUNT;item++) {
        char value[PORT_SIP_NAME_MAX + 1u];
        unsigned at=0;
        for (unsigned i=0;i<n;i++)value[at++]=translation[i];
        if(at >= PORT_SIP_NAME_MAX)return PORT_SIP_TOOLONG;
        value[at++]='[';
        for(unsigned i=0;i<top_length;i++) {
            if(at >= PORT_SIP_NAME_MAX)return PORT_SIP_TOOLONG;
            value[at++]=(char)top_system[i+1u];
        }
        if(top_length) {
            if(at >= PORT_SIP_NAME_MAX)return PORT_SIP_TOOLONG;
            value[at++]='.';
        }
        for(unsigned i=0;paths[item][i];i++) {
            if(at >= PORT_SIP_NAME_MAX)return PORT_SIP_TOOLONG;
            value[at++]=paths[item][i];
        }
        if(at >= PORT_SIP_NAME_MAX)return PORT_SIP_TOOLONG;
        value[at++]=']';
        value[at]=0;
        status=ops->create_system(context,names[item],value);
        if(!(status & 1u)) return status;
    }
    return PORT_SIP_OK;
}
