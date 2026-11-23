#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include "vms_dq_bootread.h"
#include "dq_physical_case.h"
#define FAIL_IF(cond) do { if(cond) { fprintf(stderr,"DQ physical case failed at %u\n",__LINE__);return 1;}}while(0)
static unsigned reads;
static uint32_t last_lbn;
static int read_lbn(void *p,uint32_t lbn,uint16_t sector[256]){
    (void)p;reads++;last_lbn=lbn;
    for(unsigned i=0;i<256u;i++)sector[i]=(uint16_t)(i^lbn);
    return 0;
}
int m136_physical_case(uint32_t lbn,uint32_t requested,uint32_t expected,
                       int should_read){
    uint32_t db[PORT_V37_DEVICE_BYTES/4u]={0};
    uint32_t irp[PORT_V37_IRP_SIZE/4u]={0};
    uint16_t block[256]={0};
    struct port_v37_device_result info={0};
    struct port_dq_fdt fdt={0};
    uint32_t base=(uint32_t)(uintptr_t)db;
    FAIL_IF(port_v37_devices_prelink((uint8_t *)db,sizeof db,base,&info)!=PORT_V37_DEV_OK);
    const uint32_t null_ddt=UINT32_C(0x00700000);
    port_v37_write32((uint8_t *)db+PORT_V37_DDB_NL_OFF+12u,null_ddt);
    port_v37_write32((uint8_t *)db+PORT_V37_UCB_NL_OFF+120u,null_ddt);
    info.bound_nl_ddt=null_ddt;
    info.incomplete_driver_mask=7u;
    FAIL_IF(port_v37_devices_check((uint8_t *)db,sizeof db,base,&info)!=PORT_V37_DEV_OK);
    FAIL_IF(port_dq_fdt_build(&fdt,sizeof fdt)!=PORT_DQ_FDT_OK);
    FAIL_IF(port_v37_irp_prepare((uint8_t *)irp,sizeof irp,
            (uint32_t)(uintptr_t)irp,info.ucb[0],PORT_DQ_READPBLK)!=PORT_V37_QIO_OK);
    port_v37_write16((uint8_t *)irp+42u,PORT_DQ_PHYSIO);
    port_v37_write16((uint8_t *)irp+50u,PORT_DQ_BOOTREAD_BYTES);
    port_v37_write32((uint8_t *)irp+PORT_V37_IRP_MEDIA,lbn);
    struct port_dq_bootread_request req={.fdt=&fdt,.device_db=(uint8_t *)db,
       .device_info=&info,.irp=(uint8_t *)irp,.irp_bytes=sizeof irp,
       .sector=block,.sector_bytes=sizeof block,.boot_unit=0u,
       .read_sector=read_lbn,.read_context=0};
    struct port_dq_bootread_result out={.bytes=UINT32_C(0xcafebabe)};
    const unsigned before=reads;
    uint32_t st=port_dq_bootread_execute_physical(&req,&out,requested);
    FAIL_IF(st!=expected);
    FAIL_IF(reads-before!=(unsigned)should_read);
    if(should_read){
        FAIL_IF(last_lbn!=requested||out.lbn!=requested||out.bytes!=512u||
            out.ucb_io_count!=1u||out.irp_status!=PORT_SYS85_NORMAL||
            out.sector_checksum!=port_dq_bootread_checksum(block));
        for(unsigned i=0;i<256u;i++)FAIL_IF(block[i]!=(uint16_t)(i^requested));
        /* Same UCB, fresh IRP: the resident INIT does LBN1 then LBN0. */
        uint32_t irp2[PORT_V37_IRP_SIZE/4u]={0};
        uint16_t block2[256]={0};
        FAIL_IF(port_v37_irp_prepare((uint8_t *)irp2,sizeof irp2,
             (uint32_t)(uintptr_t)irp2,info.ucb[0],PORT_DQ_READPBLK)!=PORT_V37_QIO_OK);
        port_v37_write16((uint8_t *)irp2+42u,PORT_DQ_PHYSIO);
        port_v37_write16((uint8_t *)irp2+50u,PORT_DQ_BOOTREAD_BYTES);
        port_v37_write32((uint8_t *)irp2+PORT_V37_IRP_MEDIA,0u);
        req.irp=(uint8_t *)irp2;req.sector=block2;
        struct port_dq_bootread_result followup={0};
        FAIL_IF(port_dq_bootread_execute_physical(&req,&followup,0u)!=PORT_DQ_BOOTREAD_OK);
        FAIL_IF(followup.lbn!=0u||followup.ucb_io_count!=2u||
            port_v37_read32((uint8_t *)db+PORT_V37_UCB_BOOT_OFF+
                           PORT_V37_UCB_OPCNT)!=2u||reads-before!=2u);
    }else{
        FAIL_IF(out.bytes!=UINT32_C(0xcafebabe)||
            port_v37_read32((uint8_t *)db+PORT_V37_UCB_BOOT_OFF+
                            PORT_V37_UCB_OPCNT)!=0u);
    }
    return 0;
}
int test_m136_case_00(void);
int test_m136_case_01(void);
int test_m136_case_02(void);
int test_m136_case_03(void);
int test_m136_case_04(void);
int test_m136_case_05(void);
int test_m136_case_06(void);
int test_m136_case_07(void);
int test_m136_case_08(void);
int test_m136_case_09(void);
int test_m136_case_10(void);
int test_m136_case_11(void);
int test_m136_case_12(void);
int test_m136_case_13(void);
int test_m136_case_14(void);
int test_m136_case_15(void);
int test_m136_case_16(void);
int test_m136_case_17(void);
int test_m136_case_18(void);
int test_m136_case_19(void);
int test_m136_case_20(void);
int test_m136_case_21(void);
int test_m136_case_22(void);
int test_m136_case_23(void);
int test_m136_case_24(void);
int test_m136_case_25(void);
int test_m136_case_26(void);
int test_m136_case_27(void);
int test_m136_case_28(void);
int test_m136_case_29(void);
int test_m136_case_30(void);
int test_m136_case_31(void);
int test_m136_case_32(void);
int test_m136_case_33(void);
int main(void){
    if(test_m136_case_00())return 1;
    if(test_m136_case_01())return 1;
    if(test_m136_case_02())return 1;
    if(test_m136_case_03())return 1;
    if(test_m136_case_04())return 1;
    if(test_m136_case_05())return 1;
    if(test_m136_case_06())return 1;
    if(test_m136_case_07())return 1;
    if(test_m136_case_08())return 1;
    if(test_m136_case_09())return 1;
    if(test_m136_case_10())return 1;
    if(test_m136_case_11())return 1;
    if(test_m136_case_12())return 1;
    if(test_m136_case_13())return 1;
    if(test_m136_case_14())return 1;
    if(test_m136_case_15())return 1;
    if(test_m136_case_16())return 1;
    if(test_m136_case_17())return 1;
    if(test_m136_case_18())return 1;
    if(test_m136_case_19())return 1;
    if(test_m136_case_20())return 1;
    if(test_m136_case_21())return 1;
    if(test_m136_case_22())return 1;
    if(test_m136_case_23())return 1;
    if(test_m136_case_24())return 1;
    if(test_m136_case_25())return 1;
    if(test_m136_case_26())return 1;
    if(test_m136_case_27())return 1;
    if(test_m136_case_28())return 1;
    if(test_m136_case_29())return 1;
    if(test_m136_case_30())return 1;
    if(test_m136_case_31())return 1;
    if(test_m136_case_32())return 1;
    if(test_m136_case_33())return 1;
    puts("M136 source DQ physical block READPBLK 34 cases PASS");
    return 0;
}
