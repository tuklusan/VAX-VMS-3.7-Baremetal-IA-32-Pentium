#include "vms_exe_dassgn.h"
#include "vms_exe_cancel.h"
#include <stddef.h>
uint32_t port_exe_dassgn(struct port_exe_io_channel *ch,uint32_t caller,
                         const struct port_exe_io_ops *ops) {
    if(!ch || !ch->assigned || !ch->device || ch->owner_pid!=caller ||
       !ops || !ops->driver_cancel || !ops->post_io || !ops->lock ||
       !ops->unlock || !ops->wait_io || !ops->deaccess ||
       !ops->release_volume || !ops->delete_mailbox)
        return PORT_SS_BADPARAM;
    struct port_exe_device *dev=ch->device;
    for(;;) {
        uint32_t st=port_exe_canceln(ch,caller,PORT_EXE_CANCEL_DEASSIGN,ops);
        if(!(st&1u))return st;
        if(ch->has_file){
            st=ops->deaccess(ops->context,ch);
            if(!(st&1u) && st!=PORT_SS_FILNOTACC)return st;
        }
        if(!ch->outstanding)break;
        st=ops->wait_io(ops->context,ch);
        if(!(st&1u))return st;
    }
    ops->lock(ops->context);
    if(!dev->references){ops->unlock(ops->context);return PORT_SS_BADPARAM;}
    --dev->references;
    ch->assigned=0;ch->access_mode=0;ch->owner_pid=0;
    if(!dev->references){
        dev->owner_pid=0;
        if(dev->flags&(PORT_EXE_DEV_MAILBOX|PORT_EXE_DEV_TERMINAL))
            dev->flags&=~PORT_EXE_DEV_OPERATOR;
    }else if(dev->references==1u && (dev->flags&PORT_EXE_DEV_ALLOCATED) &&
             dev->owner_pid==caller) {
        dev->flags&=~PORT_EXE_DEV_ALLOCATED;
        dev->owner_pid=0;
        --dev->references;
    }
    if((dev->flags&(PORT_EXE_DEV_MARK_DISMOUNT|PORT_EXE_DEV_FOREIGN))==
       (PORT_EXE_DEV_MARK_DISMOUNT|PORT_EXE_DEV_FOREIGN)) {
        dev->flags&=~(PORT_EXE_DEV_MARK_DISMOUNT|PORT_EXE_DEV_FOREIGN|
                      PORT_EXE_DEV_MOUNTED|PORT_EXE_DEV_SOFT_LOCK|
                      PORT_EXE_DEV_RCK|PORT_EXE_DEV_WCK|PORT_EXE_DEV_VALID);
        if(dev->volume){
            uint32_t st=ops->release_volume(ops->context,dev->volume);
            if(!(st&1u)){ops->unlock(ops->context);return st;}
            dev->volume=NULL;
        }
        dev->volume_protection=0;
    }
    uint32_t status=ops->driver_cancel(ops->context,dev,ch->channel,
                                      PORT_EXE_CANCEL_DEASSIGN);
    if(!(status&1u)){ops->unlock(ops->context);return status;}
    if(ch->associated_mailbox && dev->mailbox){
        struct port_exe_device *mb=dev->mailbox;
        dev->mailbox=NULL;
        if(!mb->references){ops->unlock(ops->context);return PORT_SS_BADPARAM;}
        --mb->references;
        status=ops->delete_mailbox(ops->context,mb);
        if(!(status&1u)){ops->unlock(ops->context);return status;}
    }
    if(dev->flags&PORT_EXE_DEV_MAILBOX){
        status=ops->delete_mailbox(ops->context,dev);
        if(!(status&1u)){ops->unlock(ops->context);return status;}
    }
    ch->device=NULL;ch->has_file=ch->associated_mailbox=0;
    ops->unlock(ops->context);
    return PORT_SS_NORMAL;
}
