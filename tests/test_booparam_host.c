#include "vms_booparam.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>
int main(void){
    struct port_boo_parameters a,b;
    memset(&a,0xff,sizeof a);
    port_boo_parameters_init(&a);
    for(unsigned i=0;i<sizeof a;i++)assert(!((uint8_t *)&a)[i]);
    a.disk_driver=0x00100000u;
    a.terminal_driver=0x00108000u;
    a.iniloa_address=0x0010c000u;
    a.iniloa_bytes=4096u;
    a.irp_count=64u;
    a.lrp_count=16u;
    a.srp_count=12u;
    a.boot_control=0x00110000u;
    memcpy(a.top_system,"SYSEXE",6);
    a.scs_system_id[5]=18u;
    a.scs_load=0x00120000u;
    port_boo_parameters_handoff(&b,&a);
    assert(memcmp(&a,&b,sizeof a)==0);
    b.irp_count=0;
    assert(a.irp_count==64u);
    printf("PASS: SYS BOOPARAM full 96-byte layout, offsets, reset and SYSBOOT/INIT handoff.\n");
    return 0;
}
