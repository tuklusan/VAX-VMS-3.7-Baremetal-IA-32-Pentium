#include "vms_sysqio_pending.h"
#include "vms_v37_dyn_tags.h"

enum { IRP_SIZE=8u,IRP_TYPE=10u,IRP_RMOD=11u,IRP_PID=12u,IRP_IOSB=36u };

static int lock_valid(const struct port_user_page_locks *locks){
    if(!locks||!locks->count||locks->count>PORT_USER_PIN_MAX_PAGES||
       !locks->physical_resolver||!locks->release||!locks->transfer_bytes||
       locks->transfer_bytes>UINT16_MAX||
       locks->virtual_base!=(locks->transfer_base&UINT32_C(0xfffff000)))
        return 0;
    const uint32_t offset=locks->transfer_base&UINT32_C(0x00000fff);
    const uint32_t required=(offset+locks->transfer_bytes+4095u)>>12u;
    if(required!=locks->count)return 0;
    for(unsigned i=0u;i<locks->count;i++)
        if((locks->physical[i]&UINT32_C(0x00000fff))||
           !locks->physical_resolver(locks->physical_context,
                                     locks->physical[i]))return 0;
    return 1;
}
static int lock_covers(const struct port_user_page_locks *locks,
                       uint32_t address,uint32_t bytes){
    return lock_valid(locks)&&address>=locks->transfer_base&&
        address-locks->transfer_base<=locks->transfer_bytes&&
        bytes<=locks->transfer_bytes-(address-locks->transfer_base);
}

static uint32_t enter(struct port_v37_sysqio_pending *table){
    return table->sync.raise_sync(table->sync.context);
}
static void leave(struct port_v37_sysqio_pending *table,uint32_t old){
    table->sync.restore_ipl(table->sync.context,old);
}
void port_v37_sysqio_pending_init(struct port_v37_sysqio_pending *table,
    const struct port_v37_irp_pool_sync *sync){
    if(!table)return;
    *table=(struct port_v37_sysqio_pending){0};
    if(sync)table->sync=*sync;
}

uint32_t port_v37_sysqio_pending_track(struct port_v37_sysqio_pending *table,
    uint32_t pid,uint32_t irp_va,const uint8_t *irp,size_t irp_bytes,
    struct port_v37_sysqio_prepared *prepared){
    if(!table||!table->sync.raise_sync||!table->sync.restore_ipl||!pid||
       !irp_va||(irp_va&3u)||!irp||irp_bytes<PORT_V37_SYSQIO_IRP_STANDARD_BYTES||
       !prepared||irp[IRP_TYPE]!=PORT_V37_DYN_IRP||
       port_v37_read16(irp+IRP_SIZE)<PORT_V37_SYSQIO_IRP_STANDARD_BYTES||
       port_v37_read16(irp+IRP_SIZE)>irp_bytes||
       port_v37_read32(irp+IRP_PID)!=pid||
       port_v37_read32(irp+IRP_IOSB)!=prepared->args.iosb||
       (prepared->args.iosb&&!lock_covers(&prepared->iosb_pages,
         prepared->args.iosb,PORT_V37_SYSQIO_IOSB_BYTES))||
       ((prepared->dispatch.action_class==PORT_DQ_FDT_READ||
         prepared->dispatch.action_class==PORT_DQ_FDT_WRITE)&&
        (uint16_t)prepared->args.parameter[1]&&
        !lock_covers(&prepared->transfer_pages,
         prepared->args.parameter[0],
         (uint16_t)prepared->args.parameter[1])))
        return PORT_V37_SYSQIO_PENDING_BAD;
    const uint8_t mode=(uint8_t)(irp[IRP_RMOD]&3u);
    const uint32_t old=enter(table);
    unsigned free_slot=PORT_V37_SYSQIO_PENDING_DEPTH;
    for(unsigned i=0u;i<PORT_V37_SYSQIO_PENDING_DEPTH;i++){
        const struct port_v37_sysqio_pending_entry *entry=&table->entries[i];
        if(!entry->state){if(free_slot==PORT_V37_SYSQIO_PENDING_DEPTH)
                free_slot=i;
            continue;
        }
        if(entry->pid==pid&&entry->irp_va==irp_va){
            leave(table,old);return PORT_V37_SYSQIO_PENDING_DUP;
        }
        /* Concurrent requests cannot share one IOSB: its completion would
         * otherwise be ambiguous even though both IRPs are valid. */
        if(prepared->args.iosb&&entry->pid==pid&&
           entry->request.args.iosb==prepared->args.iosb){
            leave(table,old);return PORT_V37_SYSQIO_PENDING_DUP;
        }
    }
    if(free_slot==PORT_V37_SYSQIO_PENDING_DEPTH){
        leave(table,old);return PORT_V37_SYSQIO_PENDING_FULL;
    }
    struct port_v37_sysqio_pending_entry *entry=&table->entries[free_slot];
    entry->request=*prepared;entry->pid=pid;entry->irp_va=irp_va;
    entry->request_mode=mode;entry->transfer_bytes=0u;
    entry->transfer_busy=0u;entry->state=1u;
    /* This is an ownership move. A caller that later aborts the request must
     * remove it through the same completion owner, never unpin the old copy. */
    prepared->transfer_pages=(struct port_user_page_locks){0};
    prepared->iosb_pages=(struct port_user_page_locks){0};
    leave(table,old);
    return PORT_V37_SYSQIO_PENDING_OK;
}

static uint32_t pending_copy_user(struct port_v37_sysqio_pending *table,
    uint32_t pid,uint32_t irp_va,uint32_t offset,void *kernel_buffer,
    uint16_t bytes,uint8_t to_user){
    if(!table||!table->sync.raise_sync||!table->sync.restore_ipl||!pid||
       !irp_va||!kernel_buffer||!bytes)
        return PORT_V37_SYSQIO_PENDING_BAD;
    const uint32_t old=enter(table);
    struct port_v37_sysqio_pending_entry *entry=0;
    for(unsigned i=0u;i<PORT_V37_SYSQIO_PENDING_DEPTH;i++){
        struct port_v37_sysqio_pending_entry *candidate=&table->entries[i];
        if(candidate->state==1u&&candidate->pid==pid&&
           candidate->irp_va==irp_va){entry=candidate;break;}
    }
    if(!entry){leave(table,old);return PORT_V37_SYSQIO_PENDING_NOTFOUND;}
    const uint8_t expected=to_user?PORT_DQ_FDT_READ:PORT_DQ_FDT_WRITE;
    const uint32_t count=(uint16_t)entry->request.args.parameter[1];
    if(entry->request.dispatch.action_class!=expected||
       offset!=entry->transfer_bytes||offset>count||bytes>count-offset){
        leave(table,old);return PORT_V37_SYSQIO_PENDING_BAD;
    }
    if(entry->transfer_busy){
        leave(table,old);return PORT_V37_SYSQIO_PENDING_DEFER;
    }
    if(entry->request.args.parameter[0]>UINT32_MAX-offset){
        leave(table,old);return PORT_V37_SYSQIO_PENDING_BAD;
    }
    entry->transfer_busy=1u;
    struct port_v37_sysqio_prepared request=entry->request;
    request.args.parameter[0]+=offset;
    request.args.parameter[1]=bytes;
    leave(table,old);

    const int copied=to_user?
        port_user_copy_to_pinned(&request.transfer_pages,
            request.args.parameter[0],kernel_buffer,bytes):
        port_user_copy_from_pinned(&request.transfer_pages,kernel_buffer,
            request.args.parameter[0],bytes);
    const uint32_t locked=enter(table);
    entry=0;
    for(unsigned i=0u;i<PORT_V37_SYSQIO_PENDING_DEPTH;i++){
        struct port_v37_sysqio_pending_entry *candidate=&table->entries[i];
        if(candidate->state==1u&&candidate->pid==pid&&
           candidate->irp_va==irp_va){entry=candidate;break;}
    }
    if(!entry||!entry->transfer_busy){
        leave(table,locked);return PORT_V37_SYSQIO_PENDING_BAD;
    }
    entry->transfer_busy=0u;
    if(copied==PORT_USER_OK){
        if(entry->transfer_bytes>UINT16_MAX-bytes){
            leave(table,locked);return PORT_V37_SYSQIO_PENDING_BAD;
        }
        entry->transfer_bytes=(uint16_t)(entry->transfer_bytes+bytes);
    }
    leave(table,locked);
    return copied==PORT_USER_OK?
        PORT_V37_SYSQIO_PENDING_OK:PORT_V37_SYSQIO_PENDING_BAD;
}

uint32_t port_v37_sysqio_pending_copy_from_user(
    struct port_v37_sysqio_pending *table,uint32_t pid,uint32_t irp_va,
    uint32_t offset,void *destination,uint16_t bytes){
    return pending_copy_user(table,pid,irp_va,offset,destination,bytes,0u);
}
uint32_t port_v37_sysqio_pending_copy_to_user(
    struct port_v37_sysqio_pending *table,uint32_t pid,uint32_t irp_va,
    uint32_t offset,const void *source,uint16_t bytes){
    return pending_copy_user(table,pid,irp_va,offset,(void *)source,bytes,1u);
}

uint32_t port_v37_sysqio_submit(struct port_v37_sysqio_pending *table,
    uint32_t pid,uint32_t irp_va,uint8_t *irp,size_t irp_bytes,
    struct port_v37_sysqio_prepared *prepared,
    port_v37_sysqio_driver_submit_fn submit,void *context){
    if(!table||!irp||!prepared||!submit||irp_bytes<IRP_RMOD+1u)
        return PORT_V37_SYSQIO_SUBMIT_BAD;
    const uint32_t tracked=port_v37_sysqio_pending_track(table,pid,irp_va,
        irp,irp_bytes,prepared);
    if(tracked!=PORT_V37_SYSQIO_PENDING_OK)
        return PORT_V37_SYSQIO_SUBMIT_TRACK;
    if(submit(context,pid,irp_va,irp,irp_bytes)==0)
        return PORT_V37_SYSQIO_SUBMIT_OK;
    const uint32_t rolled_back=port_v37_sysqio_pending_abort(table,pid,
        irp_va,port_v37_read32(irp+IRP_IOSB),(uint8_t)(irp[IRP_RMOD]&3u));
    return rolled_back==PORT_V37_SYSQIO_PENDING_OK?
        PORT_V37_SYSQIO_SUBMIT_REJECTED:PORT_V37_SYSQIO_SUBMIT_BAD;
}

uint32_t port_v37_sysqio_pending_abort(struct port_v37_sysqio_pending *table,
    uint32_t pid,uint32_t irp_va,uint32_t iosb,uint8_t mode){
    if(!table||!table->sync.raise_sync||!table->sync.restore_ipl||!pid||
       !irp_va)return PORT_V37_SYSQIO_PENDING_BAD;
    const uint32_t old=enter(table);
    struct port_v37_sysqio_pending_entry *owned=0;
    for(unsigned i=0u;i<PORT_V37_SYSQIO_PENDING_DEPTH;i++){
        struct port_v37_sysqio_pending_entry *entry=&table->entries[i];
        if(entry->state==1u&&entry->pid==pid&&entry->irp_va==irp_va&&
           entry->request.args.iosb==iosb&&entry->request_mode==mode){
            if(entry->transfer_busy){
                leave(table,old);return PORT_V37_SYSQIO_PENDING_DEFER;
            }
            owned=entry;break;
        }
    }
    if(!owned){leave(table,old);return PORT_V37_SYSQIO_PENDING_NOTFOUND;}
    owned->state=2u;
    struct port_v37_sysqio_prepared request=owned->request;
    *owned=(struct port_v37_sysqio_pending_entry){0};
    leave(table,old);
    port_user_unpin_io_range(&request.transfer_pages);
    port_user_unpin_io_range(&request.iosb_pages);
    return PORT_V37_SYSQIO_PENDING_OK;
}

int port_v37_sysqio_pending_validate(void *opaque,uint32_t pid,
    uint32_t irp_va,uint32_t iosb,uint8_t mode,uint32_t status){
    struct port_v37_sysqio_pending *table=opaque;
    if(!table||!table->sync.raise_sync||!table->sync.restore_ipl)return -1;
    const uint32_t old=enter(table);int found=0;
    for(unsigned i=0u;i<PORT_V37_SYSQIO_PENDING_DEPTH;i++){
        const struct port_v37_sysqio_pending_entry *entry=&table->entries[i];
        if(entry->state==1u&&entry->pid==pid&&entry->irp_va==irp_va&&
           entry->request.args.iosb==iosb&&entry->request_mode==mode){
            const uint16_t information=(uint16_t)(status>>16);
            const uint8_t action=entry->request.dispatch.action_class;
            const uint8_t has_buffer=(uint8_t)(
                action==PORT_DQ_FDT_READ||action==PORT_DQ_FDT_WRITE);
            found=!entry->transfer_busy&&
                (!has_buffer||(information<=
                    (uint16_t)entry->request.args.parameter[1]&&
                 information==entry->transfer_bytes));
            break;
        }
    }
    leave(table,old);return found?0:-1;
}

uint32_t port_v37_sysqio_pending_complete(void *opaque,uint32_t pid,
    uint32_t irp_va,uint32_t iosb,uint8_t mode,uint32_t status,
    uint32_t secondary){
    struct port_v37_sysqio_pending *table=opaque;
    if(!table||!table->sync.raise_sync||!table->sync.restore_ipl)
        return PORT_V37_IOPOST_BAD;
    uint32_t old=enter(table);unsigned slot=PORT_V37_SYSQIO_PENDING_DEPTH;
    for(unsigned i=0u;i<PORT_V37_SYSQIO_PENDING_DEPTH;i++){
        const struct port_v37_sysqio_pending_entry *entry=&table->entries[i];
        if(entry->state==1u&&entry->pid==pid&&entry->irp_va==irp_va&&
           entry->request.args.iosb==iosb&&entry->request_mode==mode){
            slot=i;break;
        }
    }
    if(slot==PORT_V37_SYSQIO_PENDING_DEPTH){
        leave(table,old);return PORT_V37_IOPOST_BAD;
    }
    struct port_v37_sysqio_pending_entry *entry=&table->entries[slot];
    const uint16_t information=(uint16_t)(status>>16);
    if(entry->transfer_busy){
        leave(table,old);return PORT_V37_IOPOST_DEFER;
    }
    if((entry->request.dispatch.action_class==PORT_DQ_FDT_READ||
        entry->request.dispatch.action_class==PORT_DQ_FDT_WRITE)&&
       (information>(uint16_t)entry->request.args.parameter[1]||
        information!=entry->transfer_bytes)){
        leave(table,old);return PORT_V37_IOPOST_DEFER;
    }
    entry->state=2u;
    struct port_v37_sysqio_prepared request=entry->request;
    leave(table,old);

    const struct port_v37_sysqio_iosb_value value={
        (uint16_t)status,(uint16_t)(status>>16),secondary};
    const uint32_t stored=port_v37_sysqio_copy_iosb_to_user(&request,&value);
    if(stored!=PORT_V37_SYSQIO_BUFFER_OK&&
       stored!=PORT_V37_SYSQIO_BUFFER_NONE){
        old=enter(table);entry=&table->entries[slot];entry->state=1u;
        leave(table,old);return PORT_V37_IOPOST_BAD;
    }
    port_user_unpin_io_range(&request.transfer_pages);
    port_user_unpin_io_range(&request.iosb_pages);
    old=enter(table);*entry=(struct port_v37_sysqio_pending_entry){0};
    leave(table,old);
    return PORT_V37_IOPOST_OK;
}
