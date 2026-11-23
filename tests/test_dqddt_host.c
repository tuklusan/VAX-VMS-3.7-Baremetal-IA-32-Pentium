#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include "vms_dqddt.h"
#include "vms_ini_devicedb.h"
#define CHECK(x) do {if(!(x)){fprintf(stderr,"DQ DDT failed line %u: %s\n",(unsigned)__LINE__,#x);return 1;}}while(0)
int main(void){
    enum {BASE=0x00370000u,START=0x00324120u,RET=0x00324104u};
    struct port_dq_fdt fdt={0};
    CHECK(port_dq_fdt_build(&fdt,sizeof fdt)==PORT_DQ_FDT_OK);
    struct port_dqddt_stage d={0};
    CHECK(port_dqddt_stage_init(&d,&fdt,BASE,START,RET)==PORT_DQDDT_OK);
    CHECK(d.table_base==BASE&&d.native_start_pc==START&&d.ioc_return_pc==RET);
    CHECK(port_dqddt_validate(&d,&fdt)==PORT_DQDDT_OK);
    CHECK(port_v37_read32(d.image)==(uint32_t)(START-BASE));
    CHECK(port_v37_read32(d.image+4u)==RET);
    CHECK(port_v37_read32(d.image+8u)==PORT_DQDDT_HEADER_BYTES);
    CHECK(port_v37_read16(d.image+20u)==84u);
    CHECK(port_v37_read16(d.image+36u)==136u);
    CHECK(port_dqddt_entry_offset(0u)==40u&&port_dqddt_entry_offset(1u)==48u);
    CHECK(port_dqddt_entry_offset(2u)==56u&&port_dqddt_entry_offset(11u)==164u);
    CHECK(port_dqddt_entry_offset(12u)==UINT32_MAX);
    CHECK(port_v37_read32(d.image+port_dqddt_entry_offset(2u))==(uint32_t)fdt.mask[2u]);
    CHECK(port_v37_read32(d.image+port_dqddt_entry_offset(2u)+4u)==(uint32_t)(fdt.mask[2u]>>32u));
    CHECK(port_dqddt_function(&d,&fdt,PORT_DQ_READPBLK)==PORT_DQDDT_UNBOUND);
    CHECK(port_dqddt_function(&d,&fdt,PORT_DQ_WRITELBLK)==PORT_DQDDT_UNBOUND);
    CHECK(port_dqddt_function(&d,&fdt,PORT_DQ_READVBLK)==PORT_DQDDT_UNBOUND);
    CHECK(port_dqddt_function(&d,&fdt,63u)==PORT_DQDDT_BAD);
    CHECK(port_dqddt_function(&d,&fdt,UINT16_MAX)==PORT_DQDDT_BAD);
    for(unsigned i=0;i<PORT_DQ_FDT_ROWS;i++){
        uint32_t pos=port_dqddt_entry_offset(i);
        uint8_t old=d.image[pos];d.image[pos]^=1u;
        CHECK(port_dqddt_validate(&d,&fdt)==PORT_DQDDT_BAD);
        d.image[pos]=old;
        if(i>=2u){old=d.image[pos+8u];d.image[pos+8u]=1u;
            CHECK(port_dqddt_validate(&d,&fdt)==PORT_DQDDT_BAD);
            d.image[pos+8u]=old;}
    }
    CHECK(port_dqddt_validate(&d,&fdt)==PORT_DQDDT_OK);
    d.image[20u]=0u;
    CHECK(port_dqddt_validate(&d,&fdt)==PORT_DQDDT_BAD);
    d.image[20u]=PORT_DQDDT_SOURCE_DIAG_BYTES;
    d.image[32u]=1u;
    CHECK(port_dqddt_validate(&d,&fdt)==PORT_DQDDT_BAD);
    d.image[32u]=0u;
    d.driver_bound=1u;
    CHECK(port_dqddt_validate(&d,&fdt)==PORT_DQDDT_BAD);
    d.driver_bound=0u;
    CHECK(port_dqddt_validate(&d,&fdt)==PORT_DQDDT_OK);
    struct port_dqddt_stage before=d;
    CHECK(port_dqddt_stage_init(&d,&fdt,BASE+1u,START,RET)==PORT_DQDDT_BAD);
    CHECK(!memcmp(&before,&d,sizeof d));
    CHECK(port_dqddt_stage_init(&d,&fdt,BASE,BASE+4u,RET)==PORT_DQDDT_BAD);
    CHECK(!memcmp(&before,&d,sizeof d));
    CHECK(port_dqddt_stage_init(&d,&fdt,BASE,START,BASE+8u)==PORT_DQDDT_BAD);
    CHECK(!memcmp(&before,&d,sizeof d));
    puts("DQDRIVER.MAR source DPTAB/DDTAB and 136-byte FUNCTAB stage, all ten actions unbound PASS");
    return 0;
}
