#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "vms_exe_version.h"
int main(void) {
    assert(memcmp(port_exe_version_quadword,"S1JY    ",8u)==0);
    assert(port_exe_announce_length==strlen(port_exe_announce));
    assert(strcmp(port_exe_announce,
                  "\r\n\n  VAX/VMS Version S1JY 26-APR-1982 16:21\r\n\n")==0);
    puts("VERSION.MAR release data PASS");
    return 0;
}
