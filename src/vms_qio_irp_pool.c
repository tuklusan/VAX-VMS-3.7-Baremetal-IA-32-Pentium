#include "vms_qio_irp_pool.h"
#include "vms_qio_post.h"
#include "vms_v37_dyn_tags.h"

static uint32_t rd32(const uint8_t *p){
    return (uint32_t)p[0]|((uint32_t)p[1]<<8)|
           ((uint32_t)p[2]<<16)|((uint32_t)p[3]<<24);
}
static uint16_t rd16(const uint8_t *p){
    return (uint16_t)((uint16_t)p[0]|((uint16_t)p[1]<<8));
}
static void wr32(uint8_t *p,uint32_t x){
    p[0]=(uint8_t)x;p[1]=(uint8_t)(x>>8);
    p[2]=(uint8_t)(x>>16);p[3]=(uint8_t)(x>>24);
}
static void wr16(uint8_t *p,uint16_t x){p[0]=(uint8_t)x;p[1]=(uint8_t)(x>>8);}
static uint32_t round16(uint32_t n){return (n+15u)&~15u;}

static uint8_t *packet_at(const struct port_ini_packet_list *q,uint32_t va){
    if(!q||!q->memory||va<q->split_address||!q->packet_bytes||
       (va-q->split_address)%q->packet_bytes||
       (va-q->split_address)/q->packet_bytes>=q->packet_count)return 0;
    return q->memory+(va-q->split_address);
}
static uint32_t queue_check(const struct port_ini_packet_list *q){
    if(!q||!q->packet_bytes||q->packet_bytes<16u||
       (q->packet_bytes&15u)||!q->split_address||!q->sentinel_address||
       !q->memory||q->free_count>q->packet_count)return PORT_V37_IRP_POOL_BAD;
    if(!q->free_count)
        return q->next==q->sentinel_address&&q->previous==q->sentinel_address?
            PORT_V37_IRP_POOL_OK:PORT_V37_IRP_POOL_CORRUPT;
    uint32_t prior=q->sentinel_address,at=q->next;
    for(uint32_t i=0;i<q->free_count;i++){
        uint8_t *p=packet_at(q,at);
        uint32_t bytes=p?rd16(p+8u):0u;
        if(!p||rd32(p+4u)!=prior||!bytes||round16(bytes)!=q->packet_bytes)
            return PORT_V37_IRP_POOL_CORRUPT;
        uint32_t next=rd32(p);
        if(i+1u==q->free_count){
            if(next!=q->sentinel_address||at!=q->previous)
                return PORT_V37_IRP_POOL_CORRUPT;
        }else if(!packet_at(q,next))return PORT_V37_IRP_POOL_CORRUPT;
        prior=at;at=next;
    }
    return PORT_V37_IRP_POOL_OK;
}
static int valid_sync(const struct port_v37_irp_pool_sync *s){
    return s&&s->raise_sync&&s->restore_ipl;
}

uint32_t port_v37_lookaside_alloc(struct port_ini_packet_list *q,
    uint32_t requested,uint8_t type,const struct port_v37_irp_pool_sync *sync,
    uint8_t **packet,uint32_t *address){
    if(!q||!packet||!address||!type||!valid_sync(sync)||
       requested>UINT32_MAX-15u)return PORT_V37_IRP_POOL_BAD;
    uint32_t rounded=(requested+15u)&~15u;
    if(!rounded||rounded>q->packet_bytes)return PORT_V37_IRP_POOL_BAD;
    *packet=0;*address=0u;
    uint32_t old_ipl=sync->raise_sync(sync->context);
    uint32_t st=queue_check(q);
    if(st==PORT_V37_IRP_POOL_OK&&!q->free_count)st=PORT_V37_IRP_POOL_EMPTY;
    if(st==PORT_V37_IRP_POOL_OK){
        uint32_t va=q->next;
        uint8_t *p=packet_at(q,va);
        uint32_t next=rd32(p);
        if(next==q->sentinel_address)
            q->next=q->previous=q->sentinel_address;
        else{
            uint8_t *n=packet_at(q,next);
            if(!n||rd32(n+4u)!=va)st=PORT_V37_IRP_POOL_CORRUPT;
            else{wr32(n+4u,q->sentinel_address);q->next=next;}
        }
        if(st==PORT_V37_IRP_POOL_OK){
            --q->free_count;
            wr32(p,0u);wr32(p+4u,0u);
            wr16(p+8u,(uint16_t)rounded);p[10u]=type;p[11u]=0u;
            *packet=p;*address=va;
        }
    }
    sync->restore_ipl(sync->context,old_ipl);
    return st;
}

uint32_t port_v37_lookaside_free(struct port_ini_packet_list *q,uint8_t *p,
    uint32_t va,uint32_t bytes,uint8_t type,
    const struct port_v37_irp_pool_sync *sync){
    if(!q||!p||!valid_sync(sync)||!bytes||round16(bytes)!=q->packet_bytes||
       !type||packet_at(q,va)!=p)return PORT_V37_IRP_POOL_BAD;
    uint32_t old_ipl=sync->raise_sync(sync->context);
    uint32_t st=queue_check(q);
    if(st==PORT_V37_IRP_POOL_OK){
        uint32_t cursor=q->next;
        for(uint32_t i=0;i<q->free_count;i++){
            if(cursor==va){st=PORT_V37_IRP_POOL_DUP;break;}
            cursor=rd32(packet_at(q,cursor));
        }
    }
    if(st==PORT_V37_IRP_POOL_OK&&q->free_count==q->packet_count)
        st=PORT_V37_IRP_POOL_CORRUPT;
    if(st==PORT_V37_IRP_POOL_OK){
        if(q->free_count){
            uint8_t *tail=packet_at(q,q->previous);
            if(!tail||rd32(tail)!=q->sentinel_address)
                st=PORT_V37_IRP_POOL_CORRUPT;
            else wr32(tail,va);
        }
        if(st==PORT_V37_IRP_POOL_OK){
            wr32(p,q->sentinel_address);wr32(p+4u,q->previous);
            wr16(p+8u,(uint16_t)q->packet_bytes);p[10u]=type;p[11u]=0u;
            if(!q->free_count)q->next=va;
            q->previous=va;++q->free_count;
        }
    }
    sync->restore_ipl(sync->context,old_ipl);
    return st;
}
uint32_t port_v37_irp_pool_alloc(struct port_ini_packet_list *q,
    uint32_t requested,const struct port_v37_irp_pool_sync *sync,
    uint8_t **packet,uint32_t *address){
    if(!q||q->packet_bytes<PORT_V37_IRP_SIZE||
       requested<PORT_V37_IRP_SIZE)return PORT_V37_IRP_POOL_BAD;
    return port_v37_lookaside_alloc(q,requested,PORT_V37_DYN_IRP,sync,
                                    packet,address);
}
uint32_t port_v37_irp_pool_free(struct port_ini_packet_list *q,uint8_t *p,
    uint32_t va,uint32_t bytes,const struct port_v37_irp_pool_sync *sync){
    if(!q||!p||bytes>UINT32_MAX-15u||round16(bytes)!=q->packet_bytes||
       p[10u]!=PORT_V37_DYN_IRP)
        return PORT_V37_IRP_POOL_BAD;
    return port_v37_lookaside_free(q,p,va,bytes,PORT_V37_DYN_IRP,sync);
}
