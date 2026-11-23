#include "vms_ini_bootdev_scan.h"
/* INIT.MAR counts per adapter type, starting at -1 and incrementing
 * before matching the RPB. Native INILOA publishes only probed and
 * registered adapters; confreg is the native type and sbiconf its actual
 * port base.  This is a guest-executed scan, not a VAX hardware model. */
uint32_t port_ini_bootdev_scan(const struct port_ini_io_state *mapped,
                              struct port_ini_bootdev_result *result){
    if(!mapped || !result || !mapped->num_nexus ||
       mapped->num_nexus>PORT_INI_MAX_ADAPTERS ||
       mapped->boot.boot_adapter_number>=mapped->num_nexus ||
       !mapped->boot.boot_adapter_virtual ||
       !mapped->boot.boot_csr_virtual ||
       mapped->boot.boot_adapter_virtual!=mapped->boot.boot_csr_virtual)
        return PORT_INI_BOOTDEV_BAD_ARGUMENT;
    /* Prevalidate the whole registered table before selecting any entry.
     * Rejecting duplicate I/O bases first avoids an order-sensitive RPB. */
    for(uint32_t i=0u;i<mapped->num_nexus;i++){
        if(!mapped->confreg[i] || !mapped->sbiconf[i] ||
           mapped->sbiconf[i]>65535u)
            return PORT_INI_BOOTDEV_BAD_ARGUMENT;
        for(uint32_t j=0u;j<i;j++)
            if(mapped->sbiconf[j]==mapped->sbiconf[i])
                return PORT_INI_BOOTDEV_AMBIGUOUS;
    }
    struct port_ini_bootdev_result found={0};
    uint32_t matches=0u;
    for(uint32_t i=0u;i<mapped->num_nexus;i++){
        const uint32_t type=mapped->confreg[i];
        const uint32_t io=mapped->sbiconf[i];
        /* A type's ordinal starts at zero, like the source stack -1 + INCB. */
        uint32_t unit=0u;
        for(uint32_t j=0u;j<i;j++){
            if(mapped->confreg[j]==type)unit++;
        }
        if(io!=mapped->boot.boot_csr_virtual)continue;
        matches++;
        if(i!=mapped->boot.boot_adapter_number)
            return PORT_INI_BOOTDEV_BAD_ARGUMENT;
        found.boot_index=i;
        found.boot_kind=type;
        found.boot_io_base=io;
        found.boot_kind_unit=unit;
    }
    if(matches!=1u)
        return matches?PORT_INI_BOOTDEV_AMBIGUOUS:PORT_INI_BOOTDEV_NO_MATCH;
    found.scanned_adapters=mapped->num_nexus;
    *result=found;
    return PORT_INI_BOOTDEV_OK;
}
