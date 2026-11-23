#include "vms_exe_devalc.h"
#include <stddef.h>
/* The device-table lock covers ownership, references and driver cleanup. */
static uint32_t name_copy(const struct port_exe_device *d,
                          const struct port_exe_dev_request *req,
                          const struct port_exe_device_ops *ops) {
    if(!req->physical_descriptor)return PORT_SS_NORMAL;
    uint8_t desc[8];
    if(!port_exe_read(ops->memory,req->physical_descriptor,desc,8u))
        return PORT_SS_ACCVIO;
    uint32_t size=(uint32_t)desc[0]|((uint32_t)desc[1]<<8);
    if(size>255u)size=255u;
    uint32_t dest=(uint32_t)desc[4]|((uint32_t)desc[5]<<8)|
                  ((uint32_t)desc[6]<<16)|((uint32_t)desc[7]<<24);
    if(size && !port_exe_probe_write(ops->memory,dest,size))
        return PORT_SS_ACCVIO;
    if(req->physical_length &&
       !port_exe_probe_write(ops->memory,req->physical_length,2u))
        return PORT_SS_ACCVIO;
    unsigned length=0;
    while(d->name && d->name[length] && length<255u)++length;
    uint32_t copied=length<size?length:size;
    if(copied && !port_exe_write(ops->memory,dest,d->name,copied))
        return PORT_SS_ACCVIO;
    if(req->physical_length) {
        uint16_t n=(uint16_t)copied;
        if(!port_exe_write(ops->memory,req->physical_length,&n,2u))
            return PORT_SS_ACCVIO;
    }
    return copied<length?PORT_SS_BUFFEROVF:PORT_SS_NORMAL;
}
uint32_t port_exe_alloc(struct port_exe_device *specified,
                        uint32_t pid,uint32_t uic,uint8_t previous_mode,
                        uint8_t can_alloc_spooled,
                        const struct port_exe_dev_request *req,
                        const struct port_exe_device_ops *ops) {
    if(!req || !ops || !ops->lock_io || !ops->unlock_io ||
       !ops->memory || !pid)return PORT_SS_BADPARAM;
    ops->lock_io(ops->context);
    struct port_exe_device *d=specified;
    uint32_t status=PORT_SS_NORMAL;
    if(!d) {
        if(!req->name_descriptor){status=PORT_SS_IVDEVNAM;goto out;}
        if(!port_exe_probe_read(ops->memory,req->name_descriptor,8u)){
            status=PORT_SS_ACCVIO;goto out;
        }
        if(!ops->find){status=PORT_SS_NOSUCHDEV;goto out;}
        status=ops->find(ops->context,req->name_descriptor,
                         (req->access_mode&0x100u)!=0,&d);
        if(!(status&1u))goto out;
        if(!d){status=PORT_SS_NOSUCHDEV;goto out;}
    }
    status=name_copy(d,req,ops);
    if(!(status&1u))goto out;
    if(d->owner_pid && d->owner_pid!=pid){status=PORT_SS_DEVALLOC;goto out;}
    if(!d->owner_pid){
        if((d->flags&PORT_EXE_DEV_SPOOLED) && !can_alloc_spooled){
            status=PORT_SS_NOPRIV;goto out;
        }
        if(!(d->flags&PORT_EXE_DEV_SPOOLED) && d->references){
            status=PORT_SS_DEVALLOC;goto out;
        }
        if(d->flags&PORT_EXE_DEV_MOUNTED){status=PORT_SS_DEVMOUNT;goto out;}
        if(!(d->flags&PORT_EXE_DEV_SHAREABLE) &&
           (!ops->check_read_access ||
            !(ops->check_read_access(ops->context,d,uic)&1u))) {
            status=PORT_SS_NOPRIV;goto out;
        }
    }else if(status==PORT_SS_NORMAL && (d->flags&PORT_EXE_DEV_ALLOCATED))
        status=PORT_SS_DEVALRALLOC;
    d->owner_pid=pid;
    if(!(d->flags&PORT_EXE_DEV_ALLOCATED)){
        d->flags|=PORT_EXE_DEV_ALLOCATED;
        ++d->references;
        uint8_t mode=(uint8_t)(req->access_mode&3u);
        d->alloc_mode=mode<previous_mode?previous_mode:mode;
    }
out:
    ops->unlock_io(ops->context);
    return status;
}
static uint32_t release_one(struct port_exe_device *d,uint32_t pid,
                            uint8_t mode,const struct port_exe_device_ops *ops) {
    if(!d || d->owner_pid!=pid || !(d->flags&PORT_EXE_DEV_ALLOCATED))
        return PORT_SS_DEVNOTALLOC;
    if(mode>d->alloc_mode)return PORT_SS_NOPRIV;
    if(d->references!=1u)return PORT_SS_DEVASSIGN;
    if(d->flags&PORT_EXE_DEV_MOUNTED)return PORT_SS_DEVMOUNT;
    if(!ops->driver_cancel)return PORT_SS_BADPARAM;
    uint32_t status=ops->driver_cancel(ops->context,d);
    if(!(status&1u))return status;
    d->flags&=~PORT_EXE_DEV_ALLOCATED;
    --d->references;
    d->owner_pid=0;
    if(d->flags&(PORT_EXE_DEV_TERMINAL|PORT_EXE_DEV_MAILBOX))
        d->flags&=~PORT_EXE_DEV_OPERATOR;
    return PORT_SS_NORMAL;
}
uint32_t port_exe_dalloc(uint32_t pid,uint8_t previous_mode,
                         uint32_t name_descriptor,uint32_t requested_mode,
                         const struct port_exe_device_ops *ops) {
    if(!pid || !ops || !ops->lock_io || !ops->unlock_io || !ops->memory)
        return PORT_SS_BADPARAM;
    uint8_t mode=(uint8_t)(requested_mode&3u);
    if(mode<previous_mode)mode=previous_mode;
    ops->lock_io(ops->context);
    uint32_t status=PORT_SS_NORMAL;
    if(name_descriptor){
        if(!port_exe_probe_read(ops->memory,name_descriptor,8u)){
            status=PORT_SS_ACCVIO;goto out;
        }
        if(!ops->find){status=PORT_SS_NOSUCHDEV;goto out;}
        struct port_exe_device *d=NULL;
        status=ops->find(ops->context,name_descriptor,0u,&d);
        if(!(status&1u))goto out;
        if(!d){status=PORT_SS_NOSUCHDEV;goto out;}
        status=release_one(d,pid,mode,ops);
    }else{
        for(unsigned i=0;i<ops->device_count;i++){
            struct port_exe_device *d=ops->devices[i];
            if(!d || d->owner_pid!=pid || !(d->flags&PORT_EXE_DEV_ALLOCATED) ||
               mode>d->alloc_mode || d->references!=1u)continue;
            (void)release_one(d,pid,mode,ops);
        }
        status=PORT_SS_NORMAL;
    }
out:
    ops->unlock_io(ops->context);
    return status;
}
