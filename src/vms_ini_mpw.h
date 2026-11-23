#ifndef VMS_INI_MPW_H
#define VMS_INI_MPW_H
#include <stddef.h>
#include <stdint.h>
/* SYS/SRC/INIT.MAR INI_MPW, resident modified-page-writer allocation.
 * Original MPWPFC is rounded down to a multiple of 8, with 16 used if
 * that would be zero. The allocation contains a 12-byte DYN prefix,
 * (MPWPFC+7) longword PTE entries and as many word PHV indexes. This
 * is storage setup only; page writing and swapping are not ported here.
 */
enum {PORT_INI_MPW_OK=1u,PORT_INI_MPW_BADPARAM=2u,PORT_INI_MPW_NOMEM=4u};
struct port_ini_mpw_settings {
    uint32_t requested_fault_cluster;
    uint16_t dyn_mpwmap_type; /* supplied from original DYNDEF, not guessed */
};
struct port_ini_mpw {
    const uint8_t *pte_base;
    const uint8_t *phv_index_base;
    uint32_t effective_fault_cluster, swap_increment;
    uint32_t allocated_slots, allocated_bytes;
};
uint32_t port_ini_mpw_init(void *pool,size_t capacity,
             const struct port_ini_mpw_settings *settings,
             struct port_ini_mpw *result);
uint32_t port_ini_mpw_pte(const struct port_ini_mpw *record,uint32_t i);
uint16_t port_ini_mpw_phv_index(const struct port_ini_mpw *record,uint32_t i);
#endif
