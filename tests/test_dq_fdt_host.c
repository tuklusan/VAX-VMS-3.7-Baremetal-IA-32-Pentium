#include "vms_dq_fdt.h"
#include <stdio.h>
#include <string.h>
#define CHECK(x) do{if(!(x)){fprintf(stderr,"DQ FDT test failure line %u: %s\n",__LINE__,#x);return 1;}}while(0)
static uint64_t bits(const uint8_t *list,size_t n){
    uint64_t result=0u;
    for(size_t i=0;i<n;i++)result|=PORT_DQ_BIT(list[i]);
    return result;
}
int main(void){
    struct port_dq_fdt table;
    memset(&table,0xa5,sizeof table);
    const uint8_t legal[]={0,1,2,3,4,8,10,11,12,13,14,17,26,27,32,33,35,39,48,49,50,51,52,53,54,56,57};
    const uint8_t buffered[]={0,1,2,3,4,8,17,26,27,35,39,50,51,52,53,54,56,57};
    const uint8_t read[]={14,33,12,49};
    const uint8_t write[]={10,13,32,11,48};
    const uint8_t access[]={50,51};
    const uint8_t deaccess[]={52};
    const uint8_t modify[]={56,53,54};
    const uint8_t mount[]={57};
    const uint8_t zero[]={0,1,3,4,8,17};
    const uint8_t one[]={2};
    const uint8_t sense[]={27,39};
    const uint8_t set[]={26,35};
    const uint8_t *const rows[]={legal,buffered,read,write,access,deaccess,modify,mount,zero,one,sense,set};
    const size_t lengths[]={sizeof legal,sizeof buffered,sizeof read,sizeof write,sizeof access,sizeof deaccess,sizeof modify,sizeof mount,sizeof zero,sizeof one,sizeof sense,sizeof set};
    CHECK(port_dq_fdt_build(0,sizeof table)==PORT_DQ_FDT_BAD);
    CHECK(port_dq_fdt_build(&table,sizeof table-1u)==PORT_DQ_FDT_BAD);
    CHECK(table.mask[0]==UINT64_C(0xa5a5a5a5a5a5a5a5));
    CHECK(port_dq_fdt_build(&table,sizeof table)==PORT_DQ_FDT_OK);
    CHECK(port_dq_fdt_validate(&table,sizeof table)==PORT_DQ_FDT_OK);
    uint64_t all=0;
    for(unsigned i=0;i<PORT_DQ_FDT_ROWS;i++){
        CHECK(table.mask[i]==bits(rows[i],lengths[i]));
        if(i>=PORT_DQ_FDT_READ){CHECK(!(all&table.mask[i]));all|=table.mask[i];}
    }
    CHECK(all==table.mask[PORT_DQ_FDT_LEGAL]);
    struct port_dq_fdt_decision got={0};
    for(unsigned code=0;code<64u;code++){
        const uint32_t st=port_dq_fdt_classify(&table,sizeof table,(uint16_t)code,&got);
        if(!(all&PORT_DQ_BIT(code))){
            CHECK(st==PORT_DQ_FDT_ILLEGAL);
            CHECK(port_dq_fdt_dispatch(&table,sizeof table,(uint16_t)code)==PORT_DQ_FDT_ILLEGAL);
        }else{
            CHECK(st==PORT_DQ_FDT_OK);
            CHECK(got.number==code && got.full_function==code);
            CHECK(got.buffered==!!(table.mask[PORT_DQ_FDT_BUFFERED]&PORT_DQ_BIT(code)));
            CHECK(got.action_class>=PORT_DQ_FDT_READ && got.action_class<PORT_DQ_FDT_ROWS);
            CHECK(!!(table.mask[got.action_class]&PORT_DQ_BIT(code)));
            CHECK(!got.driver_bound);
            CHECK(port_dq_fdt_dispatch(&table,sizeof table,(uint16_t)code)==PORT_DQ_FDT_UNBOUND);
        }
    }
    CHECK(port_dq_fdt_classify(&table,sizeof table,0x1000u|PORT_DQ_READLBLK,&got)==PORT_DQ_FDT_OK);
    CHECK(got.full_function==(0x1000u|PORT_DQ_READLBLK) && got.number==PORT_DQ_READLBLK &&
          got.action_class==PORT_DQ_FDT_READ && !got.buffered);
    CHECK(port_dq_fdt_classify(&table,sizeof table,PORT_DQ_WRITELBLK,&got)==PORT_DQ_FDT_OK &&
          got.action_class==PORT_DQ_FDT_WRITE && !got.buffered);
    CHECK(port_dq_fdt_classify(&table,sizeof table,PORT_DQ_MOUNT,&got)==PORT_DQ_FDT_OK &&
          got.action_class==PORT_DQ_FDT_MOUNT && got.buffered);
    uint8_t before[sizeof table];memcpy(before,&table,sizeof table);
    table.mask[PORT_DQ_FDT_WRITE]^=PORT_DQ_BIT(PORT_DQ_READLBLK);
    CHECK(port_dq_fdt_validate(&table,sizeof table)==PORT_DQ_FDT_BAD);
    CHECK(port_dq_fdt_dispatch(&table,sizeof table,PORT_DQ_READLBLK)==PORT_DQ_FDT_BAD);
    memcpy(&table,before,sizeof table);
    CHECK(port_dq_fdt_validate((const struct port_dq_fdt *)(((uint8_t *)&table)+1u),sizeof table)==PORT_DQ_FDT_BAD);
    CHECK(port_dq_fdt_classify(&table,sizeof table,PORT_DQ_READLBLK,0)==PORT_DQ_FDT_BAD);
    puts("DQDRIVER.MAR source 12 FUNCTAB masks, all 64 codes, modifiers and strict unbound policy PASS");
    return 0;
}
