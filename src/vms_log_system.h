#ifndef VMS_LOG_SYSTEM_H
#define VMS_LOG_SYSTEM_H
#include <stdint.h>
#include <stddef.h>
#include "vms_ini_fin_database.h"
/* Source LOGNAMSUB.MAR LOG$HASH/SEARCHLOG/INSLOGN/TRANSLATE subset.
 * Resident bootstrap SYSTEM names only; process/group tables, mutexes,
 * general service entry and system SYSLOGNAM are NOT implemented. */
enum { PORT_LOG_OK=1u,PORT_LOG_BAD=2u,PORT_LOG_NOTRAN=3u,
       PORT_LOG_OVERFLOW=4u,PORT_LOG_CYCLE=6u,
       PORT_LOG_SLOTS=16u,PORT_LOG_BITS=4u,
       PORT_LOG_SYSTEM=0u,PORT_LOG_MAX_NAME=63u,
       PORT_LOG_REC_LEN=PORT_INI_LOG_TEMPLATE_BYTES };
struct port_log_system_table {
    uint32_t head[PORT_LOG_SLOTS]; /* resident native bucket heads */
    uint32_t record_addr[2]; /* original 32-bit LOG$L_LTFL addresses */
    uint8_t *records[2]; /* host/guest native addresses for bounded walking */
    uint8_t hash_bits,ready;
    uint16_t count;
};
struct port_log_result {
    uint32_t status,record_address;
    uint16_t bytes;
    uint8_t table,access_mode;
};
uint32_t port_log_hash(const uint8_t *,size_t,unsigned);
uint32_t port_log_record_valid(const uint8_t *,size_t,const char *,size_t);
uint32_t port_log_system_validate(const struct port_log_system_table *,
    uint8_t *,uint8_t *,size_t);
uint32_t port_log_name_compare(const uint8_t *,size_t,const uint8_t *,size_t);
uint32_t port_log_source_address(const struct port_log_system_table *,
    const uint8_t *,uint32_t *);
uint8_t *port_log_source_record(const struct port_log_system_table *,uint32_t);
uint32_t port_log_system_insert(struct port_log_system_table *,unsigned);
uint32_t port_log_system_init(struct port_log_system_table *,
    uint8_t *,uint8_t *,size_t,unsigned);
uint32_t port_log_system_lookup(const struct port_log_system_table *,
    const uint8_t *,size_t,uint32_t *);
uint32_t port_log_system_check(const struct port_log_system_table *);
uint32_t port_log_copy_result(const uint8_t *,size_t,uint8_t *,size_t,
    uint32_t,uint32_t,uint8_t,struct port_log_result *);
uint32_t port_log_translate(const struct port_log_system_table *,
    const char *,size_t,uint32_t,uint8_t *,size_t,struct port_log_result *);
#endif
