#ifndef VMS_INI_SYSLOG_H
#define VMS_INI_SYSLOG_H
#include <stddef.h>
#include <stdint.h>
/* SYS/SRC/INIT.MAR INI_LOG: allocate and copy the prelinked SYS$DISK and
 * SYS$SYSDEVICE logical-name records to nonpaged pool. LOG$AL_LOGTBL's
 * first two cells temporarily hold their live 32-bit addresses.
 * This routine does not create the logical-name service, nor does it
 * pretend to finish device/mount/SWAPINIT initialization. */
enum {PORT_INI_SYSLOG_OK=1u,PORT_INI_SYSLOG_BADPARAM=2u,
      PORT_INI_SYSLOG_NOMEM=4u};
struct port_ini_syslog_settings {
    uint32_t size_field_offset; /* LOG$W_SIZE */
    uint32_t pool_address; /* live guest nonpaged pool address */
    uint32_t allocation_alignment; /* source-backed allocator alignment */
};
struct port_ini_syslog_result {
    uint32_t logical_table[2];
    const uint8_t *records[2];
    uint16_t sizes[2];
    uint32_t consumed_bytes;
};
uint32_t port_ini_syslog_init(uint8_t *nonpaged_pool,size_t pool_capacity,
    const uint8_t *disklog,size_t disklog_available,
    const uint8_t *sysdevicelog,size_t sysdevice_available,
    const struct port_ini_syslog_settings *settings,
    struct port_ini_syslog_result *result);
#endif
