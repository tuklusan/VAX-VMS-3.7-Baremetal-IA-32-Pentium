#include "vms_dq_ioc.h"
/* EXE$INSIOQ -> DQ_STARTIO -> RETRYERR -> FUNCXT -> IOCIOPOST.
 * Exactly one oldest queued request, retaining permanent failures as IOSB. */
uint32_t port_dq_ioc_service(struct port_dq_ioc_queue *q,
                              struct port_dq_ioc_iosb *iosb){
 if(!q||!iosb)return PORT_DQ_IOC_BAD;
 uint32_t tag=0,st=port_dq_ioc_start(q,&tag);
 if(st!=PORT_DQ_IOC_OK)return st;
 st=port_dq_ioc_retry(q);
 if(st!=PORT_DQ_IOC_OK&&st!=PORT_DQ_IOC_IO)return st;
 return port_dq_ioc_post(q,iosb);
}
