#include "vms_sysloa386.h"
/* EXE$INIBOOTADP: native boot-device check via its registered ATA driver.
 * An actual Files-11 home block at LBN 1 is necessary, not just a port ID. */
static uint16_t rd16(const uint8_t *p){
    return (uint16_t)(p[0]|((uint16_t)p[1]<<8));
}
static uint32_t rd32(const uint8_t *p){
    return (uint32_t)rd16(p)|((uint32_t)rd16(p+2)<<16);
}
static uint16_t sum16(const uint8_t *p,unsigned last){
    uint16_t s=0u;
    for(unsigned i=0;i<last;i+=2u)s=(uint16_t)(s+rd16(p+i));
    return s;
}
uint32_t port_sysloa386_check_home(const uint8_t s[512],
                                   struct port_sysloa386_boot_result *r){
    if(!s||!r)return PORT_SYSLOA386_BAD_ARGUMENT;
    /* HM2$W_STRUCLEV 2, LBN and checked sums from Files-11 boot mount. */
    if(rd32(s)!=1u||s[13]!=2u||rd16(s+14u)==0u||
       rd32(s+24u)==0u||rd32(s+28u)<rd16(s+34u)||
       rd16(s+58u)!=sum16(s,58u)||
       rd16(s+510u)!=sum16(s,510u))return PORT_SYSLOA386_BAD_DISK;
    r->home_block_lbn=rd32(s);
    r->cluster_factor=rd16(s+14u);
    r->system_structure_level=rd16(s+12u);
    r->physical_boot_sector_checksum=rd16(s+510u);
    return PORT_SYSLOA386_OK;
}
uint32_t port_sysloa386_boot_probe(const struct port_sysloa386_boot_ops *ops,
                                    struct port_sysloa386_boot_result *r){
    if(!ops||!ops->read_sector||!r)return PORT_SYSLOA386_BAD_ARGUMENT;
    uint16_t sector[256];
    if(ops->read_sector(ops->context,1u,sector))return PORT_SYSLOA386_BAD_DISK;
    /* The PC disk sector is little-endian, as are Files-11 ODS-2 headers. */
    return port_sysloa386_check_home((const uint8_t *)sector,r);
}
