#include "vms_ini_lockdb.h"
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#define CHECK(x) do{if(!(x)){fprintf(stderr,"FAIL %s:%d %s\n",__FILE__,__LINE__,#x);return 1;}}while(0)
int main(void){
    uint32_t backing[1024u];
    memset(backing,0x99,sizeof backing);
    struct port_ini_lockdb v={0};
    const struct port_ini_lock_settings s={
        .first_free_spt=1000u,.last_free_spt=1100u,.realtime_spt=35u,
        .lock_id_slots=16u,.resource_hash_requested=37u,
        .max_processes=33u,.deadlock_waittime=1u,
        .rbm_type=0x11u,.lkid_type=0x22u,.rsht_type=0x33u,
        .prcmap_type=0x44u};
    CHECK(port_ini_lockdb_init(backing,32u,&s,&v)==PORT_INI_LOCKDB_NOMEM);
    CHECK(backing[0]==0x99999999u && !v.lock_id_table);
    CHECK(port_ini_lockdb_init(backing,sizeof backing,&s,&v)==PORT_INI_LOCKDB_OK);
    CHECK(v.free_spt_vpn==1035u && v.reserved_spt_count==35u);
    CHECK(v.rt_spt_bitmap_bytes==8u);
    const uint8_t *rbm=v.rt_spt_bitmap;
    CHECK(port_ini_lockdb_read_u32(rbm)==1000u);
    CHECK(port_ini_lockdb_read_u32(rbm+4u)==35u);
    CHECK(rbm[8u]==20u && rbm[10u]==0x11u && rbm[11u]==0u);
    CHECK(port_ini_lockdb_read_u32(rbm+12u)==UINT32_MAX);
    CHECK(port_ini_lockdb_read_u32(rbm+16u)==7u);
    CHECK(v.next_lock_id==1u && v.max_lock_id==15u);
    CHECK(port_ini_lockdb_read_u32(v.lock_id_table+0u)==0u);
    CHECK(port_ini_lockdb_read_u32(v.lock_id_table+4u)==2u);
    CHECK(port_ini_lockdb_read_u32(v.lock_id_table+56u)==15u);
    CHECK(port_ini_lockdb_read_u32(v.lock_id_table+60u)==0u);
    CHECK(v.lock_id_table[-4]==76u && v.lock_id_table[-2]==0x22u);
    CHECK(v.hash_slots==64u && v.hash_power==6u);
    CHECK(v.resource_hash_table[-4]==(uint8_t)268u);
    CHECK(v.resource_hash_table[-2]==0x33u);
    for(unsigned i=0;i<v.hash_slots;i++)CHECK(port_ini_lockdb_read_u32(v.resource_hash_table+i*4u)==0u);
    CHECK(v.process_bitmap_bytes==5u && v.process_bitmap[-4]==17u);
    CHECK(v.process_bitmap[-2]==0x44u);
    for(unsigned i=0;i<v.process_bitmap_bytes;i++)CHECK(v.process_bitmap[i]==0u);
    CHECK(v.pool_bytes<=sizeof backing);
    struct port_ini_lock_settings bad=s;
    bad.realtime_spt=101u;
    uint32_t before=backing[0];
    CHECK(port_ini_lockdb_init(backing,sizeof backing,&bad,&v)==PORT_INI_LOCKDB_BADPARAM);
    CHECK(backing[0]==before);
    bad=s;bad.lock_id_slots=1u;
    CHECK(port_ini_lockdb_init(backing,sizeof backing,&bad,&v)==PORT_INI_LOCKDB_BADPARAM);
    bad=s;bad.resource_hash_requested=8193u;
    CHECK(port_ini_lockdb_init(backing,sizeof backing,&bad,&v)==PORT_INI_LOCKDB_BADPARAM);
    bad=s;bad.realtime_spt=0u;bad.deadlock_waittime=0u;
    CHECK(port_ini_lockdb_init(backing,sizeof backing,&bad,&v)==PORT_INI_LOCKDB_OK);
    CHECK(!v.rt_spt_bitmap&&!v.process_bitmap&&v.free_spt_vpn==1000u);
    puts("INIT.MAR original RT SPT bitmap, lock-id table, resource hash, deadlock process map PASS");
    return 0;
}
