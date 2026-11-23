#ifndef VMS_INI_PACKET_QUEUES_H
#define VMS_INI_PACKET_QUEUES_H
#include <stdint.h>
/* SYS/SRC/INIT.MAR INI_IRP/INI_LRP/INI_SRP, the original packet queue
 * link and IRP$W_SIZE fields use 32-bit address links at +0/+4 and a
 * 16-bit size at +8. Source-order queue insertion is after the sentinel.
 * This does not implement EXE$QIO, device scheduling or packet dispatch. */
enum { PORT_INI_PACKETS_OK=1u,PORT_INI_PACKETS_BADPARAM=2u };
struct port_ini_packet_area {
    uint8_t *memory;
    uint32_t address;         /* Actual IA-32 linear VA in a guest. */
    uint32_t available_bytes; /* Fully mapped and writable. */
    uint32_t count;
    uint32_t packet_bytes;
    uint32_t minimum_bytes;   /* LRP: BOO$GL_LRPMIN; SRP: unused. */
    uint32_t sentinel_address;/* VA of queue next/previous pair in SYS.EXE. */
};
struct port_ini_packet_list {
    uint32_t next,previous;  /* Original INSQUE-compatible links. */
    uint32_t packet_count,packet_bytes,minimum_bytes;
    uint32_t split_address,remainder_address,remainder_bytes;
    uint32_t sentinel_address,free_count;
    uint8_t *memory;
};
uint32_t port_ini_packet_init(const struct port_ini_packet_area *a,
                             struct port_ini_packet_list *out);
/* Each value is the first 12 bytes of a packet; only used for a real
 * queued packet address after validation by the owning ACP/driver. */
uint32_t port_ini_packet_first(const struct port_ini_packet_list *list,
                               uint32_t sentinel_address);
#endif
