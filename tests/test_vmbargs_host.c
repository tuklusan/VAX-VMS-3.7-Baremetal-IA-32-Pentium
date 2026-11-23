#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include "vms_vmbargs.h"
static uint32_t read_sector(void *ctx,uint32_t mode,uint32_t fn,
                            uint32_t lbn,uint32_t bytes,void *output){
    (void)ctx;(void)mode;(void)fn;(void)lbn;(void)bytes;(void)output;
    return BOO_IO_NORMAL;
}
static void test_memory(void){
    static uint8_t bitmap[PORT_VMB_MAP_BYTES];
    struct port_vmb_arguments args={0};
    const struct port_pc_memory_range ranges[]={
       {0,0,0x9fc00u,0,1},
       {0x9fc00u,0,0x60400u,0,2},
       {0x100000u,0,0x7f00000u,0,1},
       {0xfec00000u,0,0x1000u,0,2}
    };
    assert(port_vmb_memory_arguments(&args,bitmap,sizeof(bitmap),
                                     ranges,4,0x400000u)==PORT_VMB_OK);
    assert(args.count==12u && args.lowest_pfn==8192u);
    assert(args.highest_pfn==262144u);
    assert(args.bitmap_bytes==32768u);
    assert(bitmap[8191u/8u]==0 && (bitmap[8192u/8u]&1u));
    assert(bitmap[(262144u-1u)/8u]&128u);
    assert(args.flags==0 && args.microcode_bytes==0);
    const struct port_pc_memory_range bad[]={
        {0x100000u,0,0xffffffffu,0,1}
    };
    assert(port_vmb_memory_arguments(&args,bitmap,sizeof(bitmap),bad,1,0x400000u)
           ==PORT_VMB_MAP_OVERFLOW);
    assert(port_vmb_memory_arguments(&args,bitmap,sizeof(bitmap),ranges,4,0x10000000u)
           ==PORT_VMB_NO_RAM);
    const struct port_pc_memory_range overlap[]={
       {0x400000u,0,0x4000u,0,2},
       {0x400000u,0,0x10000u,0,1}
    };
    assert(port_vmb_memory_arguments(&args,bitmap,sizeof(bitmap),overlap,2,0x400000u)
           ==PORT_VMB_OK);
    assert(bitmap[8192u/8u]==0u);
    assert((bitmap[8192u/8u+8u]&1u)!=0u);
}
static void test_boot_driver(void){
    struct port_boot_driver_registration table[2];
    struct boo_io_driver disk={read_sector,0,(uint32_t)0x1234u};
    assert(port_boot_driver_register(&table[0],&disk,-1,3)==PORT_VMB_OK);
    assert(port_boot_driver_register(&table[1],&disk,-1,1)==PORT_VMB_OK);
    assert(port_boot_driver_register(&table[1],&disk,0,1)==PORT_VMB_BADARGS);
    const struct boo_io_driver *chosen=0;
    assert(port_boot_driver_select(table,2,386,1,&chosen)==PORT_VMB_OK);
    assert(chosen==&table[1].io && chosen->rpb_address==0x1234u);
    assert(chosen->transfer(0,0,1,0,512,0)==BOO_IO_NORMAL);
    assert(table[1].name[0]==4 && table[1].name[4]=='0');
    assert(port_boot_driver_select(table,2,386,7,&chosen)==PORT_VMB_NO_RAM);
    struct port_boot_driver_record rel={0};
    unsigned char action=1, image[64]={0};
    const struct port_boot_driver_specs full={
        .cpu_type=386,.device_type=1,.action=&action,.driver=image,
        .driver_bytes=sizeof(image),.entry=image+8,.name=image+12,
        .aux_name=image+20,.unit_init=image+28};
    assert(port_boot_driver_encode(&rel,&full)==PORT_VMB_OK);
    assert(rel.entry_offset==8 && rel.name_offset==12);
    assert(rel.aux_name_offset==20 && rel.unit_init_offset==28);
    assert((uintptr_t)&rel+(intptr_t)rel.action_offset==(uintptr_t)&action);
    assert((uintptr_t)&rel+(intptr_t)rel.driver_offset==(uintptr_t)image);
    table[1].record.driver_size=1;
    assert(port_boot_driver_select(table,2,386,1,&chosen)==PORT_VMB_BADARGS);
}
int main(void){test_memory();test_boot_driver();puts("PASS: VMBARGDEF and BOOT_DRIVER native page/driver tables");return 0;}
