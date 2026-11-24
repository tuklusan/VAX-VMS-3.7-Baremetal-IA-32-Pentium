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
    entry->request_mode=mode;entry->state=1u;
    /* This is an ownership move. A caller that later aborts the request must
     * remove it through the same completion owner, never unpin the old copy. */
    prepared->transfer_pages=(struct port_user_page_locks){0};
    prepared->iosb_pages=(struct port_user_page_locks){0};
    leave(table,old);
    return PORT_V37_SYSQIO_PENDING_OK;
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
    uint32_t irp_va,uint32_t iosb,uint8_t mode){
    struct port_v37_sysqio_pending *table=opaque;
    if(!table||!table->sync.raise_sync||!table->sync.restore_ipl)return -1;
    const uint32_t old=enter(table);int found=0;
    for(unsigned i=0u;i<PORT_V37_SYSQIO_PENDING_DEPTH;i++){
        const struct port_v37_sysqio_pending_entry *entry=&table->entries[i];
        if(entry->state==1u&&entry->pid==pid&&entry->irp_va==irp_va&&
           entry->request.args.iosb==iosb&&entry->request_mode==mode){
            found=1;break;
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
