#include "vms_qio.h"
static int one_flag(uint32_t mask) {
    return mask==0 || (mask&(mask-1u))==0;
}
void port_qio_init(struct port_qio_service *s,port_qio_read_backend cb,
                   void *context) {
    if(!s)return;
    *s=(struct port_qio_service){0};
    s->reader=cb;s->context=context;
}
int port_qio_submit(struct port_qio_service *s,struct port_qio_request *r,
                     struct port_qio_iosb *iosb,uint16_t sector[256],
                     uint32_t lba,uint32_t event_mask,
                     const struct vms_ast_process *owner,
                     vms_ast_callback callback,void *context,uint32_t parameter) {
    if(!s||!s->reader||!r||!iosb||!sector||!owner||!owner->pid||
       !one_flag(event_mask)||r->state) return PORT_QIO_INVALID;
    if(s->count>=PORT_QIO_MAX_QUEUED)return PORT_QIO_NO_QUOTA;
    r->next=0;r->iosb=iosb;r->sector_buffer=sector;
    r->lba=lba;r->pid=owner->pid;r->event_mask=event_mask;r->channel=0;
    r->completion=callback;r->completion_context=context;
    r->completion_parameter=parameter;
    r->ast=(struct vms_ast_request){0};
    iosb->status=0;iosb->byte_count=0;iosb->detail=0;
    r->state=1;
    if(s->tail)s->tail->next=r;else s->head=r;
    s->tail=r;++s->count;++s->submitted;
    return PORT_QIO_OK;
}
static void notify(struct port_qio_request *r,
                   struct port_event_cluster *events,
                   struct vms_ast_process *owner,
                   struct port_qio_service *s) {
    if(r->event_mask)port_event_set(events,r->event_mask);
    if(r->completion&&owner&&r->pid==owner->pid) {
        r->ast.pid=owner->pid;
        r->ast.mode=VMS_MODE_USER;
        r->ast.kind=VMS_AST_NORMAL;
        r->ast.callback=r->completion;
        r->ast.context=r->completion_context;
        r->ast.parameter=r->completion_parameter;
        if(vms_ast_enqueue(owner,&r->ast)!=VMS_AST_OK)
            ++s->ast_dropped;
    } else if(r->completion) ++s->ast_dropped;
}
int port_qio_run_one(struct port_qio_service *s,struct port_event_cluster *events,
                     struct vms_ast_process *owner) {
    if(!s||!s->reader||!events||!owner)return PORT_QIO_INVALID;
    struct port_qio_request *previous=0,*r=s->head;
    while(r&&r->pid!=owner->pid){previous=r;r=r->next;}
    if(!r)return 0;
    if(previous)previous->next=r->next;
    else s->head=r->next;
    if(s->tail==r)s->tail=previous;
    r->next=0;--s->count;
    int rc=s->reader(s->context,r->lba,r->sector_buffer);
    r->iosb->status=rc==0?PORT_QIO_IOSB_SUCCESS:PORT_QIO_IOSB_IOERROR;
    r->iosb->byte_count=rc==0?512u:0u;
    r->iosb->detail=rc==0?0:(uint32_t)rc;
    r->state=2;
    ++s->completed;
    if(rc!=0)++s->failed;
    notify(r,events,owner,s);
    return 1;
}
int port_qio_cancel(struct port_qio_service *s,struct port_qio_request *r,
                    struct port_event_cluster *events) {
    if(!s||!r||!events||r->state!=1)return PORT_QIO_INVALID;
    struct port_qio_request *prev=0,*p=s->head;
    while(p&&p!=r){prev=p;p=p->next;}
    if(!p)return PORT_QIO_INVALID;
    if(prev)prev->next=r->next;else s->head=r->next;
    if(s->tail==r)s->tail=prev;
    r->next=0;--s->count;
    r->iosb->status=PORT_QIO_IOSB_CANCELLED;
    r->iosb->byte_count=0;r->iosb->detail=0;
    r->state=2;
    ++s->cancelled;
    if(r->event_mask)port_event_set(events,r->event_mask);
    return PORT_QIO_OK;
}
int port_qio_release(struct port_qio_request *r) {
    if(!r||r->state!=2||r->ast.state!=0)return PORT_QIO_BUSY;
    r->state=0;r->next=0;
    return PORT_QIO_OK;
}
