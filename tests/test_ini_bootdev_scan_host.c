#include "vms_ini_bootdev_scan.h"
#include "vms_ini_pc_bus.h"
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#define CHECK(x) do {if(!(x)){fprintf(stderr,"FAIL %s:%d %s\n",__FILE__,__LINE__,#x);return 1;}}while(0)
int main(void){
    struct port_ini_io_state state={0};
    struct port_ini_bootdev_result got;
    memset(&got,0x6b,sizeof got);
    CHECK(port_ini_bootdev_scan(0,&got)==PORT_INI_BOOTDEV_BAD_ARGUMENT);
    CHECK(port_ini_bootdev_scan(&state,&got)==PORT_INI_BOOTDEV_BAD_ARGUMENT);
    state.num_nexus=1u;
    state.confreg[0]=PORT_INI_PC_ATA_PIO;
    state.sbiconf[0]=0x1f0u;
    state.boot.boot_adapter_number=0u;
    state.boot.boot_adapter_virtual=0x1f0u;
    state.boot.boot_csr_virtual=0x1f0u;
    CHECK(port_ini_bootdev_scan(&state,&got)==PORT_INI_BOOTDEV_OK);
    CHECK(got.boot_index==0u && got.boot_kind==PORT_INI_PC_ATA_PIO &&
          got.boot_io_base==0x1f0u && got.boot_kind_unit==0u &&
          got.scanned_adapters==1u);
    state.num_nexus=3u;
    state.confreg[1]=PORT_INI_PC_16550_UART;
    state.sbiconf[1]=0x3f8u;
    state.confreg[2]=PORT_INI_PC_ATA_PIO;
    state.sbiconf[2]=0x170u;
    state.boot.boot_adapter_number=2u;
    state.boot.boot_adapter_virtual=state.boot.boot_csr_virtual=0x170u;
    CHECK(port_ini_bootdev_scan(&state,&got)==PORT_INI_BOOTDEV_OK);
    CHECK(got.boot_kind_unit==1u && got.boot_index==2u &&
          got.boot_io_base==0x170u && got.scanned_adapters==3u);
    struct port_ini_bootdev_result unchanged=got;
    state.boot.boot_adapter_number=1u;
    CHECK(port_ini_bootdev_scan(&state,&got)==PORT_INI_BOOTDEV_BAD_ARGUMENT);
    CHECK(!memcmp(&got,&unchanged,sizeof got));
    state.boot.boot_adapter_number=2u;
    state.sbiconf[1]=0x170u;
    CHECK(port_ini_bootdev_scan(&state,&got)==PORT_INI_BOOTDEV_AMBIGUOUS);
    CHECK(!memcmp(&got,&unchanged,sizeof got));
    state.sbiconf[1]=0x3f8u;
    state.boot.boot_csr_virtual=0x180u;
    state.boot.boot_adapter_virtual=0x180u;
    CHECK(port_ini_bootdev_scan(&state,&got)==PORT_INI_BOOTDEV_NO_MATCH);
    state.boot.boot_csr_virtual=0x170u;
    CHECK(port_ini_bootdev_scan(&state,&got)==PORT_INI_BOOTDEV_BAD_ARGUMENT);
    state.boot.boot_adapter_virtual=0x170u;
    state.confreg[0]=0u;
    CHECK(port_ini_bootdev_scan(&state,&got)==PORT_INI_BOOTDEV_BAD_ARGUMENT);
    state.confreg[0]=PORT_INI_PC_ATA_PIO;
    state.num_nexus=PORT_INI_MAX_ADAPTERS+1u;
    CHECK(port_ini_bootdev_scan(&state,&got)==PORT_INI_BOOTDEV_BAD_ARGUMENT);
    CHECK(!memcmp(&got,&unchanged,sizeof got));
    puts("INIT.MAR native registered boot adapter scan, per-type units and fail-closed guards PASS");
    return 0;
}
