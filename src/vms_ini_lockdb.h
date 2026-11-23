#ifndef VMS_INI_LOCKDB_H
#define VMS_INI_LOCKDB_H
#include <stddef.h>
#include <stdint.h>
/* Original SYS/SRC/INIT.MAR INI_SPT and INI_LCKIDTBL/INI_RESHTBL/
 * INI_PRCBITMAP use 12-byte DYN records. Numeric DYN type codes must
 * come from the original VMS definitions; this interface never invents them. */
enum {PORT_INI_LOCKDB_OK=1u,PORT_INI_LOCKDB_BADPARAM=2u,
      PORT_INI_LOCKDB_NOMEM=4u};
struct port_ini_lock_settings {
    uint32_t first_free_spt, last_free_spt, realtime_spt;
    uint32_t lock_id_slots, resource_hash_requested;
    uint32_t max_processes, deadlock_waittime;
    /* Native build may leave these zero until real DYNDEF types are ported. */
    uint16_t rbm_type, lkid_type, rsht_type, prcmap_type;
};
struct port_ini_lockdb {
    const uint8_t *rt_spt_bitmap, *lock_id_table, *resource_hash_table,
                  *process_bitmap;
    uint32_t rt_spt_bitmap_bytes, id_slots, hash_slots, hash_power,
             process_bitmap_bytes, free_spt_vpn, reserved_spt_count,
             next_lock_id, max_lock_id, pool_bytes;
};
uint32_t port_ini_lockdb_init(void *pool,size_t capacity,
                   const struct port_ini_lock_settings *settings,
                   struct port_ini_lockdb *result);
uint32_t port_ini_lockdb_read_u32(const uint8_t *p);
#endif
