#include "vms_relocdrv.h"
/* Original IOC$INITDRV for DQ DPT_STORE source subset: copy real DQA0
 * DDB/UCB to bounded supervisor scratch then execute the 9 init entries.
 * Do not publish to IOC$GL_DEVLIST or mutate original DQA0 yet. */
uint32_t port_relocdrv_dq_init(struct port_relocdrv_dq *out,
    const uint8_t *device,size_t cap,const struct port_v37_device_result *info,
    const struct port_dqddt_stage *stage,const struct port_dq_fdt *fdt){
    if(!out||!device||!info||
       port_relocdrv_dq_guard(device,cap,info)!=PORT_RELOCDRV_OK||
       port_dqddt_validate(stage,fdt)!=PORT_DQDDT_OK)
        return PORT_RELOCDRV_BAD;
    struct port_relocdrv_dq tmp={0};
    tmp.source_ddb=info->ddb[0];tmp.source_ucb=info->ucb[0];
    if(!tmp.source_ddb||!tmp.source_ucb)return PORT_RELOCDRV_BAD;
    for(unsigned i=0;i<PORT_RELOCDRV_DQ_DDB_BYTES;i++)
        tmp.ddb[i]=device[PORT_V37_DDB_BOOT_OFF+i];
    for(unsigned i=0;i<PORT_RELOCDRV_DQ_UCB_BYTES;i++)
        tmp.ucb[i]=device[PORT_V37_UCB_BOOT_OFF+i];
    uint8_t table[PORT_RELOCDRV_DQ_INIT_BYTES]={0};size_t length=0;
    if(port_relocdrv_dq_table(table,sizeof table,&length)!=PORT_RELOCDRV_OK)
        return PORT_RELOCDRV_BAD;
    struct port_relocdrv_context ctx={0};
    ctx.region[0]=(struct port_relocdrv_region){tmp.ddb,
        sizeof tmp.ddb,tmp.source_ddb,PORT_V37_DYN_DDB};
    ctx.region[1]=(struct port_relocdrv_region){tmp.ucb,
        sizeof tmp.ucb,tmp.source_ucb,PORT_V37_DYN_UCB};
    ctx.dpt_base=stage->table_base;
    uint32_t count=0;
    if(port_relocdrv_apply(&ctx,table,length,&count)!=PORT_RELOCDRV_OK||
       count!=PORT_RELOCDRV_DQ_INIT_COUNT)return PORT_RELOCDRV_BAD;
    tmp.applied=count;tmp.ready=1u;
    if(port_relocdrv_dq_check(&tmp)!=PORT_RELOCDRV_OK||
       port_relocdrv_dq_footprint(&tmp,device,cap)!=PORT_RELOCDRV_OK)
        return PORT_RELOCDRV_BAD;
    *out=tmp;
    return PORT_RELOCDRV_OK;
}
