#ifndef VMS_INILOA_IOMAP_H
#define VMS_INILOA_IOMAP_H
#include <stdint.h>
/* INITADP.MAR INI$IOMAP: the IA-32 bus does not have VAX nexus registers.
 * The source outputs (CONFREG, SBICONF, NUMNEXUS, boot RPB fields) are kept;
 * architecture-specific I/O port mapping is supplied by the protected
 * driver layer. No VAX device IDs or addresses are fabricated here. */
enum {
    PORT_INI_MAP_OK=1u,
    PORT_INI_MAP_BAD_ARGUMENT=2u,
    PORT_INI_MAP_TOO_MANY=4u,
    PORT_INI_MAP_NO_BOOT_DEVICE=6u,
    PORT_INI_MAP_IO_FAILURE=8u,
    PORT_INI_MAP_PORT_CONFLICT=10u,
    PORT_INI_MAX_ADAPTERS=32u
};
struct port_ini_io_descriptor {
    uint16_t io_base;
    uint16_t io_count;
    uint16_t adapter_kind;
    uint16_t flags;
    uint32_t interrupt_vector;
};
struct port_ini_boot_rpb {
    uint32_t boot_adapter_number;
    uint32_t boot_adapter_virtual;
    uint32_t boot_csr_virtual;
};
struct port_ini_io_state {
    uint32_t num_nexus;
    uint16_t confreg[PORT_INI_MAX_ADAPTERS];
    uint32_t sbiconf[PORT_INI_MAX_ADAPTERS];
    struct port_ini_boot_rpb boot;
};
struct port_ini_io_ops {
    void *context;
    /* Probe exact device at the supplied I/O port. Odd = present.
     * An absent optional device is left unmapped; no guessed hardware. */
    uint32_t (*probe)(void *,const struct port_ini_io_descriptor *);
    /* Establish kernel-only access and initialize device registers.
     * A mapper must not claim success unless the resource is usable. */
    uint32_t (*map)(void *,const struct port_ini_io_descriptor *);
};
uint32_t port_ini_iomap(const struct port_ini_io_descriptor *adapters,
                       uint32_t adapter_count,uint16_t boot_io_base,
                       const struct port_ini_io_ops *ops,
                       struct port_ini_io_state *state);
#endif
