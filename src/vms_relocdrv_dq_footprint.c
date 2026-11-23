#include "vms_relocdrv.h"
/* Scratch DPT_STORE edits exactly original fields; all others, including
 * DDT/CRB/queue, remain byte-for-byte unchanged from source DEVICEDAT. */
static int dq_changed(unsigned off){
    return (off>=16u&&off<=19u);
}
static int ucb_changed(unsigned off){
    return off==11u||(off>=52u&&off<=56u)||(off>=58u&&off<=59u)||off==82u||off==113u||
           (off>=PORT_RELOCDRV_DQ_CURDA&&off<PORT_RELOCDRV_DQ_CURDA+4u);
}
uint32_t port_relocdrv_dq_footprint(const struct port_relocdrv_dq *d,
    const uint8_t *original,size_t cap){
    if(!d||!original||cap<PORT_V37_DEVICE_BYTES)return PORT_RELOCDRV_BAD;
    for(unsigned i=0;i<PORT_RELOCDRV_DQ_DDB_BYTES;i++){
        if(!dq_changed(i)&&d->ddb[i]!=original[PORT_V37_DDB_BOOT_OFF+i])
            return PORT_RELOCDRV_BAD;
    }
    for(unsigned i=0;i<PORT_RELOCDRV_DQ_UCB_BYTES;i++){
        if(!ucb_changed(i)&&d->ucb[i]!=original[PORT_V37_UCB_BOOT_OFF+i])
            return PORT_RELOCDRV_BAD;
    }
    return PORT_RELOCDRV_OK;
}
