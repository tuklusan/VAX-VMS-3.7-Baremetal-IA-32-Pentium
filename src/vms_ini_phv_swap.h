#ifndef VMS_INI_PHV_SWAP_H
#define VMS_INI_PHV_SWAP_H
#include <stddef.h>
#include <stdint.h>
/* SYS/SRC/INIT.MAR INI_PHV and INI_SWAP: source-form storage layout.
 * These constructors provide initialized records, not an active VMS
 * pager, PHYPCB, swapper scheduler, or pool allocator. */
enum {
    PORT_INI_PHVSWAP_OK=1u,
    PORT_INI_PHVSWAP_BADPARAM=2u,
    PORT_INI_PHVSWAP_NOMEM=4u
};
struct port_ini_phvswap_settings {
    uint32_t balance_set_count; /* SGN$GL_BALSETCT, prior to +1 system slot */
    uint32_t system_pix;        /* SGN$GW_MAXPRCCT */
    uint32_t max_working_set;   /* SGN$GL_MAXWSCNT */
    uint16_t phvec_dyn_type;    /* DYN$C_PHVEC / DYN$C_INIT encoding */
    uint16_t swapmap_dyn_type;  /* DYN$C_SWPMAP / DYN$C_INIT encoding */
};
struct port_ini_phvswap {
    const uint8_t *phv_pix_base;
    const uint8_t *phv_refcount_base;
    const uint8_t *swapper_p0br;
    uint32_t phv_slots;
    uint32_t swapper_p0lr;
    uint32_t phv_record_bytes;
    uint32_t swapmap_record_bytes;
    uint32_t pool_bytes;
};
uint32_t port_ini_phvswap_init(void *pool, size_t capacity,
                  const struct port_ini_phvswap_settings *settings,
                  struct port_ini_phvswap *out);
uint16_t port_ini_phvswap_pix(const struct port_ini_phvswap *s,uint32_t index);
int16_t port_ini_phvswap_refcount(const struct port_ini_phvswap *s,uint32_t index);
uint32_t port_ini_phvswap_pte(const struct port_ini_phvswap *s,uint32_t index);
#endif
