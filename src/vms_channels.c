#include "vms_channels.h"
/* Handle encodes generation in upper 24 bits, 1-based slot in low 8. */
static struct port_channel *lookup(struct port_channel_table *t,uint32_t handle) {
    if(!t||!handle)return 0;
    unsigned slot=(handle&255u);
    if(slot<1||slot>PORT_CHANNEL_LIMIT)return 0;
    struct port_channel *c=&t->channels[slot-1u];
    if(!c->used||c->generation!=(handle>>8))return 0;
    return c;
}
void port_channels_init(struct port_channel_table *t) {
    if(t)*t=(struct port_channel_table){0};
}
int port_channel_assign(struct port_channel_table *t,uint32_t pid,
                        port_qio_read_backend reader,void *context,uint32_t *handle) {
    if(!t||!pid||!reader||!handle)return PORT_CHANNEL_INVALID;
    for(unsigned i=0;i<PORT_CHANNEL_LIMIT;i++){
        struct port_channel *c=&t->channels[i];
        if(c->used)continue;
        uint32_t gen=(c->generation+1u)&0xFFFFFFu;
        if(!gen)gen=1;
        *c=(struct port_channel){.owner_pid=pid,.generation=gen,
                                  .reader=reader,.context=context,.used=1};
        *handle=(gen<<8)|(i+1u);
        ++t->assigned;
        return PORT_CHANNEL_OK;
    }
    ++t->denied;
    return PORT_CHANNEL_FULL;
}
int port_channel_check(const struct port_channel_table *t,uint32_t handle,
                       uint32_t pid) {
    struct port_channel *c=lookup((struct port_channel_table *)t,handle);
    if(!c)return PORT_CHANNEL_INVALID;
    return c->owner_pid==pid?PORT_CHANNEL_OK:PORT_CHANNEL_OWNER;
}
int port_channel_deassign(struct port_channel_table *t,uint32_t handle,uint32_t pid) {
    int rc=port_channel_check(t,handle,pid);
    if(rc)return rc;
    struct port_channel *c=lookup(t,handle);
    if(c->outstanding)return PORT_CHANNEL_BUSY;
    c->used=0;
    c->owner_pid=0;c->reader=0;c->context=0;
    ++t->deassigned;
    return PORT_CHANNEL_OK;
}
int port_channel_qio_submit(struct port_channel_table *t,struct port_qio_service *s,
                            uint32_t handle,struct port_qio_request *r,
                            struct port_qio_iosb *iosb,uint16_t buffer[256],
                            uint32_t lba,uint32_t event_mask,
                            const struct vms_ast_process *owner,
                            vms_ast_callback cb,void *context,uint32_t parameter) {
    if(!owner||!r)return PORT_CHANNEL_INVALID;
    int rc=port_channel_check(t,handle,owner->pid);
    if(rc)return rc;
    struct port_channel *c=lookup(t,handle);
    if(!s||s->reader!=c->reader||s->context!=c->context)
        return PORT_CHANNEL_BACKEND;
    unsigned slot=PORT_QIO_MAX_QUEUED;
    for(unsigned i=0;i<PORT_QIO_MAX_QUEUED;i++)
        if(!c->requests[i]){slot=i;break;}
    if(slot==PORT_QIO_MAX_QUEUED)return PORT_CHANNEL_BUSY;
    rc=port_qio_submit(s,r,iosb,buffer,lba,event_mask,owner,cb,context,parameter);
    if(rc)return rc;
    r->channel=handle;
    c->requests[slot]=r;
    ++c->outstanding;
    if(c->outstanding>t->peak_outstanding)t->peak_outstanding=c->outstanding;
    return PORT_CHANNEL_OK;
}
int port_channel_qio_cancel(struct port_channel_table *t,
                            struct port_qio_service *s,uint32_t handle,
                            struct port_qio_request *r,
                            struct port_event_cluster *events,uint32_t pid) {
    int rc=port_channel_check(t,handle,pid);
    if(rc)return rc;
    if(!r||r->pid!=pid||r->channel!=handle)return PORT_CHANNEL_OWNER;
    return port_qio_cancel(s,r,events);
}
int port_channel_qio_release(struct port_channel_table *t,struct port_qio_request *r) {
    if(!r||!r->channel)return PORT_CHANNEL_INVALID;
    struct port_channel *c=lookup(t,r->channel);
    if(!c||c->owner_pid!=r->pid||!c->outstanding)return PORT_CHANNEL_OWNER;
    unsigned slot=PORT_QIO_MAX_QUEUED;
    for(unsigned i=0;i<PORT_QIO_MAX_QUEUED;i++)
        if(c->requests[i]==r){slot=i;break;}
    if(slot==PORT_QIO_MAX_QUEUED)return PORT_CHANNEL_INVALID;
    int rc=port_qio_release(r);
    if(rc)return rc;
    c->requests[slot]=0;
    --c->outstanding;
    r->channel=0;
    return PORT_CHANNEL_OK;
}

int port_channels_teardown_owner(struct port_channel_table *t,
        struct port_qio_service *s,struct port_event_cluster *events,
        struct vms_ast_process *owner) {
    if(!t||!s||!events||!owner||!owner->pid)return PORT_CHANNEL_INVALID;
    /* Preflight: never tear down half a process while a callback is live. */
    for(unsigned i=0;i<PORT_CHANNEL_LIMIT;i++){
        struct port_channel *c=&t->channels[i];
        if(!c->used||c->owner_pid!=owner->pid)continue;
        uint32_t handle=(c->generation<<8)|(i+1u);
        unsigned actual=0;
        for(unsigned j=0;j<PORT_QIO_MAX_QUEUED;j++){
            struct port_qio_request *r=c->requests[j];
            if(!r)continue;
            ++actual;
            if(r->pid!=owner->pid||r->channel!=handle||
               (r->state!=1 && r->state!=2)||r->ast.state==2)
                return PORT_CHANNEL_BUSY;
        }
        if(actual!=c->outstanding)return PORT_CHANNEL_BUSY;
    }
    for(unsigned i=0;i<PORT_CHANNEL_LIMIT;i++){
        struct port_channel *c=&t->channels[i];
        if(!c->used||c->owner_pid!=owner->pid)continue;
        uint32_t handle=(c->generation<<8)|(i+1u);
        for(unsigned j=0;j<PORT_QIO_MAX_QUEUED;j++){
            struct port_qio_request *r=c->requests[j];
            if(!r)continue;
            if(r->state==1 && port_channel_qio_cancel(t,s,handle,r,events,owner->pid))
                return PORT_CHANNEL_BUSY;
            if(r->ast.state==1 && vms_ast_cancel(owner,&r->ast)!=VMS_AST_OK)
                return PORT_CHANNEL_BUSY;
            if(port_channel_qio_release(t,r))return PORT_CHANNEL_BUSY;
        }
        if(port_channel_deassign(t,handle,owner->pid))return PORT_CHANNEL_BUSY;
    }
    return PORT_CHANNEL_OK;
}
