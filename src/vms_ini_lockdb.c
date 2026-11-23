#include "vms_ini_lockdb.h"
#include <stdint.h>
/* Native IA-32 translation of INIT.MAR lines 860-992. All sizes, counts,
 * queue/bitmap offsets, power-of-two sizing and initial lock IDs follow
 * the original. This does NOT yet install system PTEs or implement $ENQ. */
static void put16(uint8_t *p,uint16_t x){p[0]=(uint8_t)x;p[1]=(uint8_t)(x>>8);}
static void put32(uint8_t *p,uint32_t x){
    for(unsigned i=0;i<4u;i++)p[i]=(uint8_t)(x>>(8u*i));
}
uint32_t port_ini_lockdb_read_u32(const uint8_t *p){
    if(!p)return 0u;
    return (uint32_t)p[0]|(uint32_t)p[1]<<8|
           (uint32_t)p[2]<<16|(uint32_t)p[3]<<24;
}
static int reserve(uint32_t *used,uint32_t size,size_t capacity,uint32_t *offset){
    uint64_t end=(uint64_t)*used+size;
    if(end>UINT32_MAX || end>capacity)return 0;
    *offset=*used;*used=(uint32_t)end;return 1;
}
uint32_t port_ini_lockdb_init(void *pool,size_t capacity,
                   const struct port_ini_lock_settings *s,
                   struct port_ini_lockdb *result){
    if(!pool || !result || !s || ((uintptr_t)pool&3u) ||
       s->lock_id_slots<2u || !s->resource_hash_requested ||
       s->lock_id_slots>16380u || s->max_processes>65535u ||
       s->realtime_spt>65535u || s->resource_hash_requested>8192u ||
       (uint64_t)s->first_free_spt+s->realtime_spt>s->last_free_spt)
        return PORT_INI_LOCKDB_BADPARAM;
    uint32_t hash_count=1u,power=0u;
    while(hash_count<s->resource_hash_requested){hash_count*=2u;++power;}
    uint32_t bitmap_words=(s->realtime_spt+31u)/32u;
    uint32_t rbm_size=s->realtime_spt?12u+4u*bitmap_words:0u;
    uint32_t id_size=12u+4u*s->lock_id_slots;
    uint32_t hash_size=12u+4u*hash_count;
    uint32_t prc_bytes=s->deadlock_waittime?(s->max_processes/8u+1u):0u;
    uint32_t proc_size=s->deadlock_waittime?12u+prc_bytes:0u;
    if(rbm_size>65535u || id_size>65535u || hash_size>65535u ||
       proc_size>65535u)return PORT_INI_LOCKDB_BADPARAM;
    uint32_t pos=0u,o_rbm=0u,o_id=0u,o_hash=0u,o_proc=0u;
    /* Plan the entire nonpaged allocation before touching any bytes. */
    if((rbm_size && !reserve(&pos,rbm_size,capacity,&o_rbm)) ||
       !reserve(&pos,id_size,capacity,&o_id) ||
       !reserve(&pos,hash_size,capacity,&o_hash) ||
       (proc_size && !reserve(&pos,proc_size,capacity,&o_proc)))
        return PORT_INI_LOCKDB_NOMEM;
    uint8_t *base=(uint8_t *)pool;
    for(uint32_t i=0;i<pos;i++)base[i]=0u;
    struct port_ini_lockdb tmp={0};
    tmp.pool_bytes=pos;
    tmp.free_spt_vpn=s->first_free_spt+s->realtime_spt;
    tmp.reserved_spt_count=s->realtime_spt;
    if(rbm_size){
        uint8_t *r=base+o_rbm;
        put32(r,s->first_free_spt); /* RBM$L_STARTVPN at byte zero. */
        put32(r+4u,s->realtime_spt);/* RBM$L_FREECOUNT at byte four. */
        put16(r+8u,(uint16_t)rbm_size);
        put16(r+10u,s->rbm_type);
        for(uint32_t i=0;i<s->realtime_spt;i++)r[12u+i/8u]|=(uint8_t)(1u<<(i&7u));
        tmp.rt_spt_bitmap=r;
        tmp.rt_spt_bitmap_bytes=4u*bitmap_words;
    }
    uint8_t *ids=base+o_id;
    put16(ids+8u,(uint16_t)id_size);
    ids[10u]=(uint8_t)s->lkid_type;
    ids[11u]=0u;
    /* Slot zero stays empty, slot 1 points to 2, final slot is zero. */
    for(uint32_t i=1u;i+1u<s->lock_id_slots;i++)put32(ids+12u+4u*i,i+1u);
    tmp.lock_id_table=ids+12u;
    tmp.id_slots=s->lock_id_slots;
    tmp.next_lock_id=1u;
    tmp.max_lock_id=s->lock_id_slots-1u;
    uint8_t *hash=base+o_hash;
    put16(hash+8u,(uint16_t)hash_size);
    hash[10u]=(uint8_t)s->rsht_type;
    hash[11u]=0u;
    tmp.resource_hash_table=hash+12u;
    tmp.hash_slots=hash_count;
    tmp.hash_power=power;
    if(proc_size){
        uint8_t *p=base+o_proc;
        put32(p+4u,prc_bytes);
        put16(p+8u,(uint16_t)proc_size);
        put16(p+10u,s->prcmap_type);
        tmp.process_bitmap=p+12u;
        tmp.process_bitmap_bytes=prc_bytes;
    }
    *result=tmp;
    return PORT_INI_LOCKDB_OK;
}
