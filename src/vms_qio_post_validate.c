#include "vms_qio_post.h"
static uint32_t read32(const uint8_t *b){return port_v37_read32(b);}
uint32_t port_v37_post_validate(const struct port_v37_iopost_queue *q){
    if(!q||!q->bytes||!q->first_irp||!q->sentinel ||
       (q->first_irp&3u)||(q->sentinel&3u)||q->packet_stride<PORT_V37_IRP_SIZE||
       (q->packet_stride&3u)||!q->packet_capacity||
       q->queued>q->packet_capacity ||
       (uint64_t)q->first_irp+(uint64_t)q->packet_capacity*q->packet_stride>UINT32_MAX ||
       ((uint64_t)q->sentinel>=q->first_irp&&
        (uint64_t)q->sentinel<q->first_irp+(uint64_t)q->packet_capacity*q->packet_stride))
        return PORT_V37_QIO_CORRUPT;
    uint32_t cursor=q->first,previous=q->sentinel;
    for(uint32_t i=0;i<q->queued;i++){
        if(cursor==q->sentinel || cursor<q->first_irp ||
           ((cursor-q->first_irp)%q->packet_stride)!=0u ||
           (cursor-q->first_irp)/q->packet_stride>=q->packet_capacity)
            return PORT_V37_QIO_CORRUPT;
        const uint8_t *packet=q->bytes+(cursor-q->first_irp);
        if(read32(packet+4u)!=previous || !read32(packet))
            return PORT_V37_QIO_CORRUPT;
        previous=cursor;cursor=read32(packet);
    }
    if(cursor!=q->sentinel || q->last!=previous ||
       (!q->queued && q->first!=q->sentinel))
        return PORT_V37_QIO_CORRUPT;
    return PORT_V37_QIO_OK;
}
