#include "vms_exe_version.h"
/* Retain the four source release bytes and the four padding spaces. */
const uint8_t port_exe_version_quadword[8]={
    'S','1','J','Y',' ',' ',' ',' '
};
/* Original Z$INITX string; it belongs after successful initialization. */
const char port_exe_announce[]=\
    "\r\n\n  VAX/VMS Version S1JY 26-APR-1982 16:21\r\n\n";
const uint32_t port_exe_announce_length=(uint32_t)(sizeof(port_exe_announce)-1u);
