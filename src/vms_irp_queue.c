#include "vms_irp_queue.h"
#include "vms_v37_dyn_tags.h"

static uint32_t rd32(const uint8_t *p){
    return (uint32_t)p[0]|((uint32_t)p[1]<<8)|
        ((uint32_t)p[2]<<16)|((uint32_t)p[3]<<24);
}
static uint16_t rd16(const uint8_t *p){
    return (uint16_t)((uint16_t)p[0]|((uint16_t)p[1]<<8));
}
static void wr32(uint8_t *p,uint32_t v){
    p[0]=(uint8_t)v;p[1]=(uint8_t)(v>>8);
    p[2]=(uint8_t)(v>>16);p[3]=(uint8_t)(v>>24);
}
static uint32_t head_va(const struct port_v37_irp_queue *q){
    return q->ucb_va+PORT_V37_UCB_IOQFL;
}
static int node(struct port_v37_irp_queue *q,uint32_t va,uint8_t **p,
    size_t *bytes){
    if(va==head_va(q)){
        *p=q->ucb+PORT_V37_UCB_IOQFL;*bytes=q->ucb_bytes-
            PORT_V37_UCB_IOQFL;return 0;
    }
    return q->resolve(q->context,va,p,bytes);
}
static int valid_irp(struct port_v37_irp_queue *q,uint32_t va,
    uint8_t **irp,size_t *bytes){
    return va&&!(va&3u)&&va!=head_va(q)&&node(q,va,irp,bytes)==0&&
        *irp&&*bytes>=PORT_V37_IRP_MIN_BYTES&&
        rd16(*irp+8u)>=PORT_V37_IRP_MIN_BYTES&&
        rd16(*irp+8u)<=*bytes&&(*irp)[10u]==PORT_V37_DYN_IRP;
}
uint32_t port_v37_irp_queue_init(struct port_v37_irp_queue *q,uint8_t *ucb,
    size_t ucb_bytes,uint32_t ucb_va,uint32_t maximum_entries,
    port_v37_irp_resolve_fn resolve,void *context){
    if(!q||!ucb||ucb_bytes<PORT_V37_UCB_IOQFL+8u||!ucb_va||
       (ucb_va&3u)||ucb_va>UINT32_MAX-PORT_V37_UCB_IOQFL||
       !maximum_entries||!resolve||((uintptr_t)ucb&3u))
        return PORT_V37_IRP_QUEUE_BAD;
    *q=(struct port_v37_irp_queue){ucb,ucb_bytes,ucb_va,maximum_entries,
        resolve,context};
    wr32(ucb+PORT_V37_UCB_IOQFL,head_va(q));
    wr32(ucb+PORT_V37_UCB_IOQFL+4u,head_va(q));
    return PORT_V37_IRP_QUEUE_OK;
}
uint32_t port_v37_irp_queue_validate(const struct port_v37_irp_queue *q,
    uint32_t *count_out){
    if(!q||!q->ucb||!q->resolve||!q->maximum_entries||
       q->ucb_bytes<PORT_V37_UCB_IOQFL+8u||!q->ucb_va||
       (q->ucb_va&3u)||q->ucb_va>UINT32_MAX-PORT_V37_UCB_IOQFL||
       ((uintptr_t)q->ucb&3u))
        return PORT_V37_IRP_QUEUE_BAD;
    const uint32_t h=head_va(q);uint8_t *head=q->ucb+PORT_V37_UCB_IOQFL;
    uint32_t prev=h,at=rd32(head),count=0u;
    while(at!=h){
        uint8_t *irp=0;size_t bytes=0u;
        if(count>=q->maximum_entries||!at||(at&3u)||at==prev||
           q->resolve(q->context,at,&irp,&bytes)!=0||!irp||
           bytes<PORT_V37_IRP_MIN_BYTES||
           rd16(irp+8u)<PORT_V37_IRP_MIN_BYTES||rd16(irp+8u)>bytes||
           irp[10u]!=PORT_V37_DYN_IRP||
           rd32(irp+PORT_V37_IRP_IOQBL)!=prev)
            return PORT_V37_IRP_QUEUE_CORRUPT;
        uint8_t *prior=0;size_t prior_bytes=0u;
        if(node((struct port_v37_irp_queue *)q,prev,&prior,&prior_bytes)!=0||
           !prior||rd32(prior+PORT_V37_IRP_IOQFL)!=at)
            return PORT_V37_IRP_QUEUE_CORRUPT;
        prev=at;at=rd32(irp+PORT_V37_IRP_IOQFL);count++;
    }
    if(rd32(head+4u)!=prev){return PORT_V37_IRP_QUEUE_CORRUPT;}
    uint8_t *tail=0;size_t tail_bytes=0u;
    if(node((struct port_v37_irp_queue *)q,prev,&tail,&tail_bytes)!=0||
       !tail||rd32(tail)!=h)return PORT_V37_IRP_QUEUE_CORRUPT;
    if(count_out)*count_out=count;
    return PORT_V37_IRP_QUEUE_OK;
}
uint32_t port_v37_irp_queue_insert(struct port_v37_irp_queue *q,uint32_t va){
    uint8_t *irp=0;size_t irp_bytes=0u;
    if(!q||!valid_irp(q,va,&irp,&irp_bytes)||
       rd32(irp+PORT_V37_IRP_IOQFL)||rd32(irp+PORT_V37_IRP_IOQBL))
        return PORT_V37_IRP_QUEUE_BAD;
    uint32_t count=0u;
    if(port_v37_irp_queue_validate(q,&count)!=PORT_V37_IRP_QUEUE_OK)
        return PORT_V37_IRP_QUEUE_CORRUPT;
    if(count>=q->maximum_entries)return PORT_V37_IRP_QUEUE_FULL;
    const uint32_t h=head_va(q);uint32_t after=h;
    for(uint32_t at=rd32(q->ucb+PORT_V37_UCB_IOQFL+4u);at!=h;){
        uint8_t *cur=0;size_t bytes=0u;
        if(q->resolve(q->context,at,&cur,&bytes)!=0||!cur||
           bytes<PORT_V37_IRP_MIN_BYTES)return PORT_V37_IRP_QUEUE_CORRUPT;
        /* SYSQIOREQ CMPB/BLSSU walks backward while new priority is
         * unsigned-less-than the current entry. Lower numeric priorities
         * precede higher ones; equal priorities remain FIFO. */
        if(irp[PORT_V37_IRP_PRI]>=cur[PORT_V37_IRP_PRI]){after=at;break;}
        at=rd32(cur+PORT_V37_IRP_IOQBL);
    }
    uint8_t *pred=0,*succ=0;size_t pred_bytes=0u,succ_bytes=0u;
    if(node(q,after,&pred,&pred_bytes)!=0||!pred)return PORT_V37_IRP_QUEUE_CORRUPT;
    const uint32_t next=rd32(pred+PORT_V37_IRP_IOQFL);
    if(node(q,next,&succ,&succ_bytes)!=0||!succ)return PORT_V37_IRP_QUEUE_CORRUPT;
    wr32(irp,next);wr32(irp+4u,after);
    wr32(pred,va);wr32(succ+4u,va);
    return PORT_V37_IRP_QUEUE_OK;
}
uint32_t port_v37_irp_queue_remove(struct port_v37_irp_queue *q,uint32_t va){
    uint8_t *irp=0;size_t bytes=0u;
    if(!q||!valid_irp(q,va,&irp,&bytes))return PORT_V37_IRP_QUEUE_BAD;
    uint32_t count=0u;
    if(port_v37_irp_queue_validate(q,&count)!=PORT_V37_IRP_QUEUE_OK)
        return PORT_V37_IRP_QUEUE_CORRUPT;
    uint32_t at=rd32(q->ucb+PORT_V37_UCB_IOQFL);
    while(at!=head_va(q)&&at!=va){
        uint8_t *cur=0;size_t n=0u;
        if(q->resolve(q->context,at,&cur,&n)!=0||!cur)
            return PORT_V37_IRP_QUEUE_CORRUPT;
        at=rd32(cur+PORT_V37_IRP_IOQFL);
    }
    if(at!=va)return PORT_V37_IRP_QUEUE_NOTFOUND;
    const uint32_t prev=rd32(irp+4u),next=rd32(irp);
    uint8_t *pred=0,*succ=0;size_t pn=0u,sn=0u;
    if(node(q,prev,&pred,&pn)!=0||node(q,next,&succ,&sn)!=0||!pred||!succ)
        return PORT_V37_IRP_QUEUE_CORRUPT;
    wr32(pred,next);wr32(succ+4u,prev);wr32(irp,0u);wr32(irp+4u,0u);
    return PORT_V37_IRP_QUEUE_OK;
}
