#include "vms_relocdrv.h"
/* Exact *data*, not original MACRO code: DQDRIVER.MAR 406-419 DPT_STORE
 * INIT records. DDB ACPD=F11/PACK; UCB FIPL/DEVCHAR/DEVCLASS/DEVBUFSIZ,
 * DIPL/ERTMAX/CURDA. Source REINIT requires real CRB/IDB and stays unbound. */
uint32_t port_relocdrv_dq_table(uint8_t *out,size_t cap,size_t *used){
    static const uint8_t data[]={
        PORT_V37_DYN_DDB,16u,PORT_RELOCDRV_OP_LONG,'F','1','1',0u,
        PORT_V37_DYN_DDB,19u,PORT_RELOCDRV_OP_BYTE,1u,
        PORT_V37_DYN_UCB,11u,PORT_RELOCDRV_OP_BYTE,8u,
        PORT_V37_DYN_UCB,52u,PORT_RELOCDRV_OP_LONG,0x08u,0x40u,0x45u,0x1cu,
        PORT_V37_DYN_UCB,56u,PORT_RELOCDRV_OP_BYTE,1u,
        PORT_V37_DYN_UCB,58u,PORT_RELOCDRV_OP_WORD,0u,2u,
        PORT_V37_DYN_UCB,82u,PORT_RELOCDRV_OP_BYTE,21u,
        PORT_V37_DYN_UCB,113u,PORT_RELOCDRV_OP_BYTE,8u,
        PORT_V37_DYN_UCB,PORT_RELOCDRV_DQ_CURDA,PORT_RELOCDRV_OP_LONG,
          0xffu,0xffu,0xffu,0xffu,
        0u};
    if(!out||!used||sizeof data!=PORT_RELOCDRV_DQ_INIT_BYTES||
       cap<sizeof data)return PORT_RELOCDRV_BAD;
    for(size_t i=0;i<sizeof data;i++)out[i]=data[i];
    *used=sizeof data;return PORT_RELOCDRV_OK;
}
