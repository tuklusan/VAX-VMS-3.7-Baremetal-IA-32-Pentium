#ifndef VMS_EXE_MEMORY85_H
#define VMS_EXE_MEMORY85_H
#include <stdint.h>
#include "vms_userptr.h"

enum { PORT_M85_NORMAL=1u, PORT_M85_ACCVIO=12u, PORT_M85_LENVIO=396u,
       PORT_M85_PAGE_BYTES=4096u };

/* Native P0, P1 and system address spaces; IA-32 uses 4-KiB PTEs. */
struct port_m85_regions {
    uint32_t first_free_p0;
    uint32_t first_free_p1;
    uint32_t first_free_system;
};
struct port_m85_pte {
    uint32_t va;
    uint32_t vpn;
    uint32_t pde_index;
    uint32_t pte_index;
    uint32_t *pte;
    uint32_t value;
};
typedef uint32_t *(*port_m85_fault_pte_fn)(void *,uint32_t);
typedef void (*port_m85_fatal_fn)(void *,uint32_t);
struct port_m85_pte_ops {
    const struct port_user_map *map;
    struct port_m85_regions regions;
    port_m85_fault_pte_fn page_table_fault;
    port_m85_fatal_fn fatal_length_violation;
    void *context;
};
uint32_t port_mmg_pteindex(const struct port_m85_pte_ops *,uint32_t,
                            struct port_m85_pte *);
uint32_t port_mmg_pteref(const struct port_m85_pte_ops *,uint32_t,
                          struct port_m85_pte *);
uint32_t port_mmg_svaptechk(const struct port_m85_pte_ops *,uint32_t,
                             struct port_m85_pte *);
uint32_t port_mmg_pteadrchk(const struct port_m85_pte_ops *,uint32_t,
                             struct port_m85_pte *);

struct port_m85_buffer {
    struct port_user_map map;
    port_phys_page_resolver physical;
    void *physical_context;
    uint32_t address;
    uint32_t current_pte_index;
    uint32_t current_page_phys;
    uint8_t *window;
    uint8_t writable;
};
uint32_t port_ioc_filspt(struct port_m85_buffer *);
uint32_t port_ioc_initbufwind(struct port_m85_buffer *,uint32_t,int);
uint32_t port_ioc_getbyte(struct port_m85_buffer *,uint8_t *);
uint32_t port_ioc_putbyte(struct port_m85_buffer *,uint8_t);
uint32_t port_ioc_movfruser(struct port_m85_buffer *,uint8_t *,uint32_t);
uint32_t port_ioc_movtouser(struct port_m85_buffer *,const uint8_t *,uint32_t);
#endif
