#include "vms_iniloa_iomap.h"
/* Original INI$IOMAP loops over a CPU nexus table, maps recognized adapter
 * windows, and publishes CONFREG and SBICONF along with the RPB boot link.
 * IA-32 has no SBI/UNIBUS nexus. Instead, the installed x86 platform
 * supplies an explicit ISA/PCI-I/O-port descriptor table; the port never
 * touches unknown I/O ports or reports an adapter it did not probe. */
uint32_t port_ini_iomap(const struct port_ini_io_descriptor *adapters,
                       uint32_t adapter_count,uint16_t boot_io_base,
                       const struct port_ini_io_ops *ops,
                       struct port_ini_io_state *state){
    if(!adapters||!ops||!ops->probe||!ops->map||!state||
       !adapter_count||adapter_count>PORT_INI_MAX_ADAPTERS||
       !boot_io_base)return PORT_INI_MAP_BAD_ARGUMENT;
    /* Validate before mapping: a bad descriptor must not publish half an
     * initialized RPB. I/O ranges are inclusive-exclusive uint32 intervals. */
    uint32_t boot_count=0u;
    for(uint32_t i=0;i<adapter_count;i++){
        const struct port_ini_io_descriptor *a=&adapters[i];
        if(!a->io_count||!a->adapter_kind||
           (uint32_t)a->io_base+a->io_count>0x10000u)
            return PORT_INI_MAP_BAD_ARGUMENT;
        if(a->io_base==boot_io_base)boot_count++;
        for(uint32_t j=0;j<i;j++){
            const struct port_ini_io_descriptor *b=&adapters[j];
            if((uint32_t)a->io_base<(uint32_t)b->io_base+b->io_count&&
               (uint32_t)b->io_base<(uint32_t)a->io_base+a->io_count)
                return PORT_INI_MAP_PORT_CONFLICT;
        }
    }
    if(boot_count!=1u)return PORT_INI_MAP_NO_BOOT_DEVICE;
    struct port_ini_io_state work={0};
    uint32_t found_boot=0u;
    for(uint32_t i=0;i<adapter_count;i++){
        const struct port_ini_io_descriptor *a=&adapters[i];
        uint32_t present=ops->probe(ops->context,a);
        if(!(present&1u)){
            if(a->io_base==boot_io_base)return PORT_INI_MAP_NO_BOOT_DEVICE;
            continue;
        }
        uint32_t rc=ops->map(ops->context,a);
        if(!(rc&1u))return PORT_INI_MAP_IO_FAILURE;
        uint32_t at=work.num_nexus++;
        work.confreg[at]=a->adapter_kind;
        work.sbiconf[at]=a->io_base;
        if(a->io_base==boot_io_base){
            work.boot.boot_adapter_number=at;
            work.boot.boot_adapter_virtual=a->io_base;
            work.boot.boot_csr_virtual=a->io_base;
            found_boot=1u;
        }
    }
    if(!found_boot)return PORT_INI_MAP_NO_BOOT_DEVICE;
    *state=work;
    return PORT_INI_MAP_OK;
}
