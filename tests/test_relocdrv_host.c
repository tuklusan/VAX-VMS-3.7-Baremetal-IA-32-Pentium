#include "vms_relocdrv.h"
#include <stdio.h>
#include <string.h>
#define CHECK(x) do{if(!(x)){fprintf(stderr,"RELOCDRV FAILED line %u: %s\n",\
    (unsigned)__LINE__,#x);return 1;}}while(0)
int main(void){
    enum {DB_BASE=0x00360000u,DDT_BASE=0x00370000u,
          START=0x00324120u,RET=0x00324104u};
    uint32_t source_words[PORT_V37_DEVICE_BYTES/4u]={0};
    uint8_t *source=(uint8_t *)source_words;
    struct port_v37_device_result info={0};
    CHECK(port_v37_devices_prelink(source,sizeof source_words,DB_BASE,&info)==PORT_V37_DEV_OK);
    info.incomplete_driver_mask=7u;info.boot_name_ready=1u;
    CHECK(port_relocdrv_dq_guard(source,PORT_V37_DEVICE_BYTES,&info)==PORT_RELOCDRV_OK);
    struct port_dq_fdt fdt={0};
    struct port_dqddt_stage stage={0};
    CHECK(port_dq_fdt_build(&fdt,sizeof fdt)==PORT_DQ_FDT_OK);
    CHECK(port_dqddt_stage_init(&stage,&fdt,DDT_BASE,START,RET)==PORT_DQDDT_OK);
    uint8_t prior[PORT_V37_DEVICE_BYTES]={0};
    memcpy(prior,source,sizeof prior);
    struct port_relocdrv_dq dq={0};
    CHECK(port_relocdrv_dq_init(&dq,source,PORT_V37_DEVICE_BYTES,&info,&stage,&fdt)==PORT_RELOCDRV_OK);
    CHECK(port_relocdrv_dq_check(&dq)==PORT_RELOCDRV_OK);
    CHECK(port_relocdrv_dq_footprint(&dq,source,PORT_V37_DEVICE_BYTES)==PORT_RELOCDRV_OK);
    uint8_t preserved_devtype=dq.ucb[57u];dq.ucb[57u]^=1u;
    CHECK(port_relocdrv_dq_footprint(&dq,source,PORT_V37_DEVICE_BYTES)==PORT_RELOCDRV_BAD);
    dq.ucb[57u]=preserved_devtype;
    CHECK(memcmp(prior,source,sizeof prior)==0); /* not yet published */
    CHECK(dq.source_ddb==info.ddb[0]&&dq.source_ucb==info.ucb[0]&&dq.applied==9u);
    uint8_t tab[PORT_RELOCDRV_DQ_INIT_BYTES]={0};size_t used=0;
    CHECK(port_relocdrv_dq_table(tab,sizeof tab,&used)==PORT_RELOCDRV_OK);
    CHECK(used==PORT_RELOCDRV_DQ_INIT_BYTES&&tab[0]==PORT_V37_DYN_DDB&&tab[used-1u]==0u);
    CHECK(port_relocdrv_dq_table(tab,used-1u,&used)==PORT_RELOCDRV_BAD);
    CHECK(port_relocdrv_dq_guard(source,PORT_V37_DEVICE_BYTES-1u,&info)==PORT_RELOCDRV_UNBOUND);
    uint32_t original=info.incomplete_driver_mask;
    info.incomplete_driver_mask=15u;
    CHECK(port_relocdrv_dq_init(&dq,source,PORT_V37_DEVICE_BYTES,&info,&stage,&fdt)==PORT_RELOCDRV_BAD);
    info.incomplete_driver_mask=original;
    uint32_t ddt=port_v37_read32(source+PORT_V37_DDB_BOOT_OFF+12u);
    port_v37_write32(source+PORT_V37_DDB_BOOT_OFF+12u,0x00370000u);
    CHECK(port_relocdrv_dq_guard(source,PORT_V37_DEVICE_BYTES,&info)==PORT_RELOCDRV_UNBOUND);
    port_v37_write32(source+PORT_V37_DDB_BOOT_OFF+12u,ddt);
    uint8_t ddb[52]={0};uint8_t ucb[252]={0};
    const uint32_t ro_value=0x1234abcd;
    uint8_t ro[8]={0};port_relocdrv_write(ro,4u,ro_value);
    struct port_relocdrv_context c={0};
    c.region[0]=(struct port_relocdrv_region){ddb,sizeof ddb,0x00360000u,PORT_V37_DYN_DDB};
    c.region[1]=(struct port_relocdrv_region){ucb,sizeof ucb,0x00361000u,PORT_V37_DYN_UCB};
    c.readonly_bytes=ro;c.readonly_size=sizeof ro;c.readonly_base=0x00364000u;
    c.dpt_base=0x00380000u;
    uint32_t done=0;
    const uint8_t ops[]={
       PORT_V37_DYN_DDB,0u,PORT_RELOCDRV_OP_BYTE,0xa5u,
       PORT_V37_DYN_DDB,2u,PORT_RELOCDRV_OP_WORD,0x34u,0x12u,
       PORT_V37_DYN_DDB,4u,PORT_RELOCDRV_OP_DISPLACED,0xfcu,0xffu,
       PORT_V37_DYN_UCB,4u,PORT_RELOCDRV_OP_LONG,0x78u,0x56u,0x34u,0x12u,
       PORT_V37_DYN_UCB,8u,PORT_RELOCDRV_OP_FIELD,0x0bu,0u,0u,0u,3u,5u,
       PORT_V37_DYN_UCB,12u,PORT_RELOCDRV_OP_INDIRECT|PORT_RELOCDRV_OP_WORD,
          0u,0x40u,0x36u,0u,
       PORT_V37_DYN_UCB,16u,PORT_RELOCDRV_OP_INDIRECT|PORT_RELOCDRV_OP_FIELD,
          0u,0x40u,0x36u,0u,0u,32u,
       0u};
    CHECK(port_relocdrv_apply(&c,ops,sizeof ops,&done)==PORT_RELOCDRV_OK&&done==7u);
    CHECK(ddb[0]==0xa5u&&port_relocdrv_read16(ddb+2u)==0x1234u);
    CHECK(port_relocdrv_read32(ddb+4u)==0x0037fffcu);
    CHECK(port_relocdrv_read32(ucb+4u)==0x12345678u);
    CHECK(port_relocdrv_read32(ucb+8u)==0x58u);
    CHECK(port_relocdrv_read16(ucb+12u)==0xabcdu);
    CHECK(port_relocdrv_read32(ucb+16u)==ro_value);
    uint8_t before[sizeof ucb];memcpy(before,ucb,sizeof before);
    const uint8_t malformed[]={PORT_V37_DYN_UCB,9u,PORT_RELOCDRV_OP_BYTE,99u,
       PORT_V37_DYN_UCB,250u,PORT_RELOCDRV_OP_LONG,1u,2u,3u,4u,0u};
    CHECK(port_relocdrv_apply(&c,malformed,sizeof malformed,&done)==PORT_RELOCDRV_BAD);
    CHECK(!memcmp(before,ucb,sizeof before)&&done==7u);
    const uint8_t bad_tag[]={41u,4u,PORT_RELOCDRV_OP_BYTE,1u,0u};
    CHECK(port_relocdrv_apply(&c,bad_tag,sizeof bad_tag,&done)==PORT_RELOCDRV_BAD);
    const uint8_t bad_field[]={PORT_V37_DYN_UCB,0u,PORT_RELOCDRV_OP_FIELD,
       0xffu,0u,0u,0u,31u,2u,0u};
    CHECK(port_relocdrv_apply(&c,bad_field,sizeof bad_field,&done)==PORT_RELOCDRV_BAD);
    const uint8_t bad_indirect[]={PORT_V37_DYN_UCB,1u,
       PORT_RELOCDRV_OP_INDIRECT,0xffu,0xffu,0xffu,0xffu,0u};
    CHECK(port_relocdrv_apply(&c,bad_indirect,sizeof bad_indirect,&done)==PORT_RELOCDRV_UNBOUND);
    CHECK(!memcmp(before,ucb,sizeof before));
    CHECK(port_relocdrv_apply(&c,ucb,sizeof ucb,&done)==PORT_RELOCDRV_BAD);
    CHECK(port_relocdrv_field(ucb,1u,32u,1u,1)==PORT_RELOCDRV_BAD);
    CHECK(port_relocdrv_field(ucb,1u,0u,0u,1)==PORT_RELOCDRV_BAD);
    const uint8_t empty[]={0u};
    CHECK(port_relocdrv_apply(&c,empty,sizeof empty,&done)==PORT_RELOCDRV_OK&&done==0u);
    /* Separate host arrays can still alias in guest physical address space. */
    struct port_relocdrv_context alias=c;
    alias.region[1].pc_base=alias.region[0].pc_base+4u;
    CHECK(port_relocdrv_isolated(&alias,ops,sizeof ops)==PORT_RELOCDRV_BAD);
    alias=c;alias.region[1].pc_base=UINT32_MAX-1u;
    CHECK(port_relocdrv_isolated(&alias,ops,sizeof ops)==PORT_RELOCDRV_BAD);
    alias=c;alias.readonly_base=alias.region[0].pc_base+2u;
    CHECK(port_relocdrv_isolated(&alias,ops,sizeof ops)==PORT_RELOCDRV_BAD);
    alias=c;alias.readonly_base=UINT32_MAX-1u;
    CHECK(port_relocdrv_isolated(&alias,ops,sizeof ops)==PORT_RELOCDRV_BAD);
    alias=c;alias.readonly_base=0u;
    CHECK(port_relocdrv_isolated(&alias,ops,sizeof ops)==PORT_RELOCDRV_BAD);
    alias=c;alias.readonly_bytes=NULL;
    CHECK(port_relocdrv_isolated(&alias,ops,sizeof ops)==PORT_RELOCDRV_BAD);
    CHECK(port_relocdrv_apply(&alias,ops,sizeof ops,&done)==PORT_RELOCDRV_BAD);
    CHECK(done==0u);
    CHECK(port_relocdrv_isolated(&c,ops,sizeof ops)==PORT_RELOCDRV_OK);
    puts("RELOCDRV.MAR source IOC$INITDRV DPT_STORE interpreter, original DQ defaults and strict no-DDT-bind PASS");
    return 0;
}
