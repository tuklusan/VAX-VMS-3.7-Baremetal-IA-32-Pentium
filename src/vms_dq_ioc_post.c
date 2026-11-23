#include "vms_dq_ioc.h"
/* IOCIOPOST: copy final status before retiring IRP; one post per request.
 * Successful packets alone publish 512 bytes to caller's destination. */
uint32_t port_dq_ioc_post(struct port_dq_ioc_queue *q,
                           struct port_dq_ioc_iosb *iosb){
 if(!q||!iosb||q->posted==UINT32_MAX)return PORT_DQ_IOC_BAD;
 struct port_dq_ioc_iosb candidate;
 uint32_t st=port_dq_ioc_iosb(q,&candidate);
 if(st!=PORT_DQ_IOC_OK)return st;
 uint32_t tag=0;
 st=port_dq_ioc_complete(q,&tag);
 if(tag!=candidate.tag)return PORT_DQ_IOC_CORRUPT;
 q->posted++;*iosb=candidate;
 return st;
}
