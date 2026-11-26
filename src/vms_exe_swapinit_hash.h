#ifndef VMS_EXE_SWAPINIT_HASH_H
#define VMS_EXE_SWAPINIT_HASH_H
#include <stdint.h>
#include "vms_exe_paged_pool.h"
#include "vms_log_system.h"

enum {
    PORT_EXE_SWAPINIT_HASH_OK=1u,
    PORT_EXE_SWAPINIT_HASH_BAD=2u,
    PORT_EXE_SWAPINIT_HASH_NOMEM=4u,
    PORT_EXE_SWAPINIT_HASH_CORRUPT=6u,
    PORT_EXE_SWAPINIT_HASH_DUP=8u,
    PORT_EXE_SWAPINIT_HASH_TABLES=3u,
    PORT_EXE_SWAPINIT_RSHT_HEADER_BYTES=12u,
    PORT_EXE_SWAPINIT_INITIAL_NAMES=2u
};
struct port_exe_swapinit_name_record {
    uint8_t *bytes;
    uint32_t address;
};
struct port_exe_swapinit_hash_table {
    uint8_t *record;
    uint8_t *buckets;
    uint32_t record_address;
    uint32_t requested_entries,entry_count,allocated_bytes;
    struct port_exe_swapinit_name_record linked_names[
        PORT_EXE_SWAPINIT_INITIAL_NAMES];
    uint8_t entry_log2,linked_count,reserved[2];
};
struct port_exe_swapinit_hash_set {
    struct port_exe_swapinit_hash_table table[PORT_EXE_SWAPINIT_HASH_TABLES];
    void *owned_records[PORT_EXE_SWAPINIT_INITIAL_NAMES];
    uint32_t owned_record_bytes[PORT_EXE_SWAPINIT_INITIAL_NAMES];
};

/* SWAPPER.MAR EXE$SWAPINIT table sizing and 30$ record allocation. Index 0
 * is SYSTEM, 1 GROUP, 2 PROCESS, matching LOG$GL_HTBLSIZ[R8]. The two
 * existing SYS$DISK records are linked separately by LOG$INSLOGN. `out`
 * must initially be zeroed or hold a set already returned by release(). */
uint32_t port_exe_swapinit_hash_init(struct port_exe_paged_pool *,
    const uint32_t requested_entries[PORT_EXE_SWAPINIT_HASH_TABLES],
    struct port_exe_swapinit_hash_set *);
uint32_t port_exe_swapinit_hash_release(struct port_exe_paged_pool *,
    struct port_exe_swapinit_hash_set *);
/* SWAPPER.MAR's two LOG$INSLOGN calls for the prebuilt SYSTEM records.
 * Supports only SYS$DISK and SYS$SYSDEVICE; general logical-name services,
 * table locks, access modes and supersession remain outside this entry. */
uint32_t port_exe_swapinit_insert_system_record(
    struct port_exe_swapinit_hash_set *,uint8_t *,uint32_t);
/* Bounded LOG$SEARCHLOG walk over the live SWAPPER SYSTEM RSHT. This table
 * contains the two initial SYS$DISK/SYS$SYSDEVICE records only; returns the
 * PORT_LOG_* status family. */
uint32_t port_exe_swapinit_search_system(
    const struct port_exe_swapinit_hash_set *,const uint8_t *,size_t,
    uint32_t *record_address);
#endif
