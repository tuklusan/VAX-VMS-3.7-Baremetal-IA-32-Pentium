#include "vms_sysmount.h"

static uint32_t status_ok(uint32_t status){return status&1u;}
static uint32_t setbit(uint64_t *options,uint32_t bit){
    if(bit>=64u)return 0;
    *options|=(uint64_t)1u<<bit;
    return 1;
}
void port_sysmount_reset(struct port_sysmount_state *state){
    if(!state)return;
    *state=(struct port_sysmount_state){0};
    setbit(&state->mount_options,PORT_MOUNT_OPT_SYSTEM);
    setbit(&state->mount_options,PORT_MOUNT_OPT_WRITE);
    setbit(&state->mount_options,PORT_MOUNT_OPT_BLOCK);
    setbit(&state->mount_options,PORT_MOUNT_OPT_OVR_ID);
    setbit(&state->mount_options,PORT_MOUNT_OPT_DEVICE);
    setbit(&state->mount_options,PORT_MOUNT_OPT_LABEL);
    setbit(&state->mount_options,PORT_MOUNT_OPT_MOUNTVER);
    state->device_count=1;
    state->label_count=1;
}
/* This has the same _!AC!UW: result as SYSMOU's $FAO call. */
static uint32_t physical_name(char out[24],const char name[12],uint16_t unit){
    size_t len=0,n=0;
    out[len++]='_';
    while(n<12u && name[n]){
        if(len+7u>=24u)return PORT_SYSMOUNT_BAD_ARGUMENT;
        out[len++]=name[n++];
    }
    if(!n || n==12u)return PORT_SYSMOUNT_BAD_ARGUMENT;
    char numbers[5];
    uint32_t digits=0,number=unit;
    do{numbers[digits++]=(char)('0'+number%10u);number/=10u;}while(number);
    for(uint32_t i=0;i<digits;i++)out[len++]=numbers[digits-1u-i];
    out[len++]=':';out[len]=0;
    return 1u;
}
static uint32_t devices_equal(const struct port_sysmount_device *a,
                               const struct port_sysmount_device *b){
    const unsigned char *x=(const unsigned char *)a,*y=(const unsigned char *)b;
    for(size_t i=0;i<sizeof(*a);i++)if(x[i]!=y[i])return 0u;
    return 1u;
}
uint32_t port_sysmount_system(struct port_sysmount_state *state,
                              const struct port_sysmount_services *services,
                              const struct port_sysmount_status *codes,
                              uint32_t system_channel){
    if(!state||!services||!codes)return PORT_SYSMOUNT_BAD_ARGUMENT;
    port_sysmount_reset(state);
    state->channel=system_channel;
    /* $GETCHN, including the check against a spooled or mismatched disk. */
    if(!services->getchn||!services->read_home ||
       !services->mount_disk1||!services->mount_disk2)
        return PORT_SYSMOUNT_NO_SERVICE;
    uint32_t status=services->getchn(services->context,system_channel,
                                     &state->primary,&state->secondary);
    if(!status_ok(status))return status;
    if(!devices_equal(&state->primary,&state->secondary)||
       !(state->primary.flags&PORT_SYSMOUNT_FOD)||
       (state->primary.flags&PORT_SYSMOUNT_SQD))return codes->not_file_dev;
    if(!(state->primary.flags&PORT_SYSMOUNT_AVL))return codes->device_offline;
    if(state->primary.flags&PORT_SYSMOUNT_MNT)return codes->device_mounted;
    status=physical_name(state->physical_name,state->primary.name,
                          state->primary.unit);
    if(!status_ok(status))return status;
    /* The source uses 'SYSTEMDISK' as the default volume label; /OVR_ID
     * makes an incorrect label a non-fatal READ_HOMEBLOCK result. */
    status=services->read_home(services->context,system_channel,
                               "SYSTEMDISK",state);
    setbit(&state->mount_options,PORT_MOUNT_OPT_IS_FILES11);
    if(!status_ok(status) && status!=codes->incorrect_label)return status;
    if(state->level2)setbit(&state->mount_options,PORT_MOUNT_OPT_IS_FILES11B);
    status=state->level2 ? services->mount_disk2(services->context,state):
                            services->mount_disk1(services->context,state);
    if(!status_ok(status))return status;
    /* Original MOUNT_DISK1/2 must mount and start the ACP.  A bare
     * FIL$MOUNT call is insufficient and must not reach LOGINOUT. */
    if(!state->volume_mounted||!state->acp_started)
        return PORT_SYSMOUNT_INCOMPLETE;
    if((state->cleanup_flags&(1u<<PORT_MOUNT_CLF_REBUILD))!=0u){
        if(!services->rebuild)return PORT_SYSMOUNT_NO_SERVICE;
        if(services->message_rebuild)services->message_rebuild(services->context);
        status=services->rebuild(services->context,state->channel,
                   (state->cleanup_flags&(1u<<PORT_MOUNT_CLF_REBUILDQUO))!=0u);
        if(!status_ok(status))return status;
    }
    return 1u;
}
uint32_t port_sysmount_main_handler(struct port_sysmount_state *state,
                                    const struct port_sysmount_services *services,
                                    const struct port_sysmount_status *codes,
                                    uint32_t signal_status,
                                    uint32_t *saved_r0){
    if(!state||!services||!codes||!saved_r0)return PORT_SYSMOUNT_BAD_ARGUMENT;
    /* VMS STS$V_SEVERITY spans three low bits; severe is value four.
     * SS$_UNWIND must not start another unwind. */
    if(signal_status==codes->unwind || (signal_status&7u)!=4u)
        return codes->continue_status;
    *saved_r0=signal_status ? signal_status : state->user_status[0];
    if(!services->unwind)return PORT_SYSMOUNT_NO_SERVICE;
    services->unwind(services->context);
    /* $UNWIND does not return here on successful execution. */
    return PORT_SYSMOUNT_UNWIND_RETURN;
}
