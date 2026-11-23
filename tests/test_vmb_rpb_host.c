#include "vms_vmb_rpb.h"
#include <stdio.h>
#include <stdlib.h>

static uint8_t pfn_map[PORT_VMB_MAP_BYTES];
static void check(int value,const char *message){
    if(!value){fprintf(stderr,"FAIL: %s\n",message);exit(1);}
}
static uint32_t transfer(void *ctx,uint32_t mode,uint32_t operation,
                         uint32_t lbn,uint32_t count,void *buffer){
    (void)ctx;(void)mode;(void)operation;(void)lbn;(void)count;(void)buffer;
    return 1;
}
int main(void){
    const struct port_pc_memory_range ranges[]={
      {.base_lo=0,.size_lo=0x9fc00u,.type=1},
      {.base_lo=0x100000u,.size_lo=0x07f00000u,.type=1},
      {.base_lo=0x9fc00u,.size_lo=0x60400u,.type=2}
    };
    const struct boo_io_driver disk={.transfer=transfer};
    const struct port_vmb_boot_registers pc_boot={1u,0x1f0u,0x1f7u,0u,0u,0u};
    struct port_vmb_arguments args;
    struct port_vmb_restart_block rpb;
    uint32_t rc=port_vmb_memory_arguments(&args,pfn_map,sizeof(pfn_map),ranges,3,0x400000u);
    check(rc==PORT_VMB_OK,"E820 map");
    rc=port_vmb_init_restart(&rpb,&args,pfn_map,sizeof(pfn_map),ranges,3,&disk,&pc_boot);
    check(rc==PORT_VMB_OK,"RPB initialize");
    check(rpb.base_address==(uint32_t)(uintptr_t)&rpb,"RPB self address");
    check(rpb.pfn_bitmap_bytes==args.bitmap_bytes&&
          rpb.pfn_bitmap_address==args.bitmap_address,"RPB PFN map descriptor");
    check(rpb.memory_descriptor_count==2u&&rpb.memory[0].start_pfn==0&&
          rpb.memory[0].pages==0x9fc00u/512u,"first BIOS region");
    check(rpb.memory[1].start_pfn==0x100000u/512u&&
          rpb.memory[1].pages==0x7f00000u/512u,"second BIOS region");
    check(rpb.free_page_count==(0x8000000u-0x400000u)/512u,"reserved pages excluded");
    check(rpb.pfn_low==args.lowest_pfn&&rpb.pfn_high==args.highest_pfn,"PFN bounds");
    check(rpb.boot_driver==&disk&&rpb.boot_device_type==1&&rpb.boot_unit==0,
          "boot driver metadata");
    check(rpb.boot_registers[0]==1u && rpb.boot_registers[1]==0x1f0u &&
          rpb.boot_registers[2]==0x1f7u && rpb.boot_registers[3]==0u,
          "native incoming boot registers");
    check(port_vmb_init_restart(&rpb,&args,pfn_map,sizeof(pfn_map),ranges,3,0,&pc_boot)==
          PORT_VMB_RPB_NO_DRIVER,"no driver must stop boot");
    uint32_t bit=args.lowest_pfn/8u;
    pfn_map[bit] &= (uint8_t)~(1u<<(args.lowest_pfn&7u));
    check(port_vmb_init_restart(&rpb,&args,pfn_map,sizeof(pfn_map),ranges,3,&disk,&pc_boot)==
          PORT_VMB_OK,"one bad page stays unavailable");
    pfn_map[0]=1;
    check(port_vmb_init_restart(&rpb,&args,pfn_map,sizeof(pfn_map),ranges,3,&disk,&pc_boot)==
          PORT_VMB_RPB_BAD_MAP,"no reserved page in free map");
    puts("PASS: VMB restart block, E820 descriptors, PFN count, reserved RAM and boot-driver selection");
    return 0;
}
