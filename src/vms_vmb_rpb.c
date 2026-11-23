#include "vms_vmb_rpb.h"

/* A 512-byte VMS page is used for every RPB page count, including on PC. */
uint32_t port_vmb_init_restart(struct port_vmb_restart_block *rpb,
                          const struct port_vmb_arguments *args,
                          const uint8_t *bitmap,uint32_t bitmap_capacity,
                          const struct port_pc_memory_range *e820,
                          uint32_t e820_count,
                          const struct boo_io_driver *driver,
                          const struct port_vmb_boot_registers *boot) {
    if(!rpb||!args||!bitmap||!boot||!e820||!e820_count||
       e820_count>PORT_VMB_E820_LIMIT||!bitmap_capacity||
       bitmap_capacity>PORT_VMB_MAP_BYTES ||
       args->bitmap_bytes>bitmap_capacity||!args->bitmap_bytes||
       args->bitmap_address!=(uint32_t)(uintptr_t)bitmap||
       boot->device_type>UINT8_MAX||boot->unit>UINT16_MAX||
       args->highest_pfn>bitmap_capacity*8u||
       args->lowest_pfn>=args->highest_pfn)
        return PORT_VMB_RPB_BAD_ARGUMENT;
    if(!driver||!driver->transfer)return PORT_VMB_RPB_NO_DRIVER;
    *rpb=(struct port_vmb_restart_block){0};
    rpb->base_address=(uint32_t)(uintptr_t)rpb;
    rpb->pfn_bitmap_bytes=args->bitmap_bytes;
    rpb->pfn_bitmap_address=args->bitmap_address;
    rpb->pfn_low=args->lowest_pfn;
    rpb->pfn_high=args->highest_pfn;
    rpb->boot_driver=driver;
    rpb->boot_unit=(uint16_t)boot->unit;
    rpb->boot_device_type=(uint8_t)boot->device_type;
    rpb->boot_registers[0]=boot->device_type;
    rpb->boot_registers[1]=boot->adapter_port;
    rpb->boot_registers[2]=boot->controller_port;
    rpb->boot_registers[3]=boot->unit;
    rpb->boot_registers[4]=boot->adapter_flags;
    rpb->boot_registers[5]=boot->software_flags;
    /* Check the PFN map itself.  Never assign pages to SYSBOOT solely
     * because BIOS reported that memory is present. */
    for(uint32_t page=0;page<bitmap_capacity*8u;page++){
        if(!(bitmap[page>>3u]&(1u<<(page&7u))))continue;
        if(page<args->lowest_pfn||page>=args->highest_pfn)
            return PORT_VMB_RPB_BAD_MAP;
        rpb->free_page_count++;
    }
    if(!rpb->free_page_count)return PORT_VMB_RPB_BAD_MAP;
    /* The RPB memory descriptors list physical E820 type-one ranges.
     * Reserved RAM remains absent from the independent free PFN map. */
    for(uint32_t i=0;i<e820_count;i++){
        const struct port_pc_memory_range *range=&e820[i];
        if(range->type!=1u)continue;
        uint64_t base=(uint64_t)range->base_lo |
                            ((uint64_t)range->base_hi<<32u);
        uint64_t bytes=(uint64_t)range->size_lo |
                            ((uint64_t)range->size_hi<<32u);
        if(!bytes||base>UINT64_MAX-bytes)return PORT_VMB_RPB_BAD_ARGUMENT;
        uint64_t start=base/512u+((base%512u)!=0u);
        uint64_t end=(base+bytes)/512u;
        if(end<=start)continue;
        if(start>UINT32_MAX||end>UINT32_MAX)return PORT_VMB_RPB_MANY_RANGES;
        if(rpb->memory_descriptor_count>=PORT_VMB_RPB_MAX_RANGES)
            return PORT_VMB_RPB_MANY_RANGES;
        struct port_vmb_memory_descriptor *d=
              &rpb->memory[rpb->memory_descriptor_count++];
        d->start_pfn=(uint32_t)start;
        d->pages=(uint32_t)(end-start);
    }
    if(!rpb->memory_descriptor_count)return PORT_VMB_RPB_BAD_MAP;
    return PORT_VMB_OK;
}
