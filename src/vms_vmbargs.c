#include "vms_vmbargs.h"

/* VMB.MAR's good-page map is built from the native BIOS memory list.
 * BIOS E820 is the PC replacement for probing VAX memory controllers.
 * A page is accepted only if its entire 512 bytes are usable, and it is
 * outside the running kernel's reserved early physical address range. */
uint32_t port_vmb_memory_arguments(struct port_vmb_arguments *args,
                                  uint8_t *map,uint32_t capacity,
                                  const struct port_pc_memory_range *ranges,
                                  uint32_t count,uint32_t reserved_end) {
    if(!args||!map||!ranges||!count||count>PORT_VMB_E820_LIMIT||
       !capacity||capacity>PORT_VMB_MAP_BYTES)return PORT_VMB_BADARGS;
    for(uint32_t i=0;i<capacity;i++)map[i]=0;
    const uint64_t limit=(uint64_t)capacity*8u*PORT_VMB_PAGE_SIZE;
    /* E820 type one marks good memory. Every other type overrides it,
     * irrespective of entry order. Pages intersecting reserved regions
     * cannot be put on the free PFN map. */
    for(unsigned pass=0;pass<2;pass++){
        for(uint32_t i=0;i<count;i++){
            const struct port_pc_memory_range *r=&ranges[i];
            if((r->type==1u)!=(pass==0u))continue;
            uint64_t base=(uint64_t)r->base_lo | ((uint64_t)r->base_hi<<32);
            uint64_t size=(uint64_t)r->size_lo | ((uint64_t)r->size_hi<<32);
            if(!size)continue;
            if(r->type==1u && (base>=limit || size>limit-base))
                return PORT_VMB_MAP_OVERFLOW;
            if(base>=limit)continue;
            uint64_t end=size>limit-base ? limit : base+size;
            uint64_t first=(pass==0u) ? (base+511u)/512u : base/512u;
            uint64_t last=(pass==0u) ? end/512u : (end+511u)/512u;
            if(last>capacity*8u)last=capacity*8u;
            for(uint64_t page=first;page<last;page++){
                uint8_t mask=(uint8_t)(1u << ((unsigned)page&7u));
                if(pass==0u)map[(unsigned)page>>3u]|=mask;
                else map[(unsigned)page>>3u]&=(uint8_t)~mask;
            }
        }
    }
    uint32_t reserved_pages=reserved_end/512u+(reserved_end%512u!=0u);
    if(reserved_pages>capacity*8u)reserved_pages=capacity*8u;
    for(uint32_t page=0;page<reserved_pages;page++)
        map[page>>3u]&=(uint8_t)~(1u<<(page&7u));
    uint32_t lowest=UINT32_MAX,highest=0;
    for(uint32_t byte=0;byte<capacity;byte++)if(map[byte]){
        for(uint32_t bit=0;bit<8u;bit++)if(map[byte]&(1u<<bit)){
            uint32_t page=byte*8u+bit;
            if(page<lowest)lowest=page;
            if(page>=highest)highest=page+1u;
        }
    }
    if(lowest==UINT32_MAX || highest<=lowest)return PORT_VMB_NO_RAM;
    uint32_t used_bytes=(highest+7u)/8u;
    args->count=12u;
    args->cache_bytes=0;
    args->cache_address=0;
    args->lowest_pfn=lowest;
    args->highest_pfn=highest;
    args->bitmap_bytes=used_bytes;
    args->bitmap_address=(uint32_t)(uintptr_t)map;
    args->microcode_bytes=0;
    args->microcode_address=0;
    for(uint32_t j=0;j<6u;j++)args->scs_system_id[j]=0;
    args->spare=0;
    args->flags=0;
    args->ci_highest_pfn=0;
    return PORT_VMB_OK;
}

/* Relative table cells match $BOOT_DRIVER's assembler expressions.
 * Optional routines and names use zero when the source macro omits them. */
static uint32_t relative_cell(const void *base,const void *target,
                              int32_t *delta){
    if(!target){*delta=0;return PORT_VMB_OK;}
    int64_t value=(int64_t)(uint64_t)(uintptr_t)target-
                  (int64_t)(uint64_t)(uintptr_t)base;
    if(value<INT32_MIN || value>INT32_MAX)return PORT_VMB_BADARGS;
    *delta=(int32_t)value;
    return PORT_VMB_OK;
}
uint32_t port_boot_driver_encode(struct port_boot_driver_record *out,
                                 const struct port_boot_driver_specs *specs){
    if(!out||!specs||specs->cpu_type==0||!specs->driver||
       !specs->driver_bytes||!specs->name)return PORT_VMB_BADARGS;
    struct port_boot_driver_record temp={0};
    temp.cpu_type=specs->cpu_type;
    temp.device_type=specs->device_type;
    temp.driver_size=specs->driver_bytes;
    if(relative_cell(out,specs->action,&temp.action_offset)!=PORT_VMB_OK ||
       relative_cell(out,specs->driver,&temp.driver_offset)!=PORT_VMB_OK ||
       relative_cell(specs->driver,specs->entry,&temp.entry_offset)!=PORT_VMB_OK ||
       relative_cell(specs->driver,specs->name,&temp.name_offset)!=PORT_VMB_OK ||
       relative_cell(specs->driver,specs->aux_name,&temp.aux_name_offset)!=PORT_VMB_OK ||
       relative_cell(specs->driver,specs->unit_init,&temp.unit_init_offset)!=PORT_VMB_OK)
        return PORT_VMB_BADARGS;
    *out=temp;
    return PORT_VMB_OK;
}
uint32_t port_boot_driver_register(struct port_boot_driver_registration *out,
                               const struct boo_io_driver *io,
                               int16_t cpu_type,int16_t device_type){
    if(!out||!io || cpu_type==0)return PORT_VMB_BADARGS;
    out->io=*io;
    out->name[0]=4;out->name[1]='I';out->name[2]='D';
    out->name[3]='E';out->name[4]='0';
    struct port_boot_driver_specs specs={cpu_type,device_type,0,&out->io,
                                        0,out->name,0,0,sizeof(out->io)};
    return port_boot_driver_encode(&out->record,&specs);
}
uint32_t port_boot_driver_select(const struct port_boot_driver_registration *table,
                                 size_t count,int16_t cpu_type,int16_t device_type,
                                 const struct boo_io_driver **driver){
    if(!table||!driver || !count)return PORT_VMB_BADARGS;
    *driver=0;
    for(size_t i=0;i<count;i++){
        const struct port_boot_driver_registration *candidate=&table[i];
        const struct port_boot_driver_record *entry=&candidate->record;
        if(entry->cpu_type!=cpu_type && entry->cpu_type!=-1)continue;
        if(entry->device_type!=device_type && entry->device_type!=-1)continue;
        if(entry->driver_offset!=(int32_t)offsetof(struct port_boot_driver_registration,io)||
           entry->name_offset!=(int32_t)(offsetof(struct port_boot_driver_registration,name)-
                                         offsetof(struct port_boot_driver_registration,io))||
           entry->driver_size!=sizeof(struct boo_io_driver))return PORT_VMB_BADARGS;
        *driver=&candidate->io;
        return PORT_VMB_OK;
    }
    return PORT_VMB_NO_RAM;
}
