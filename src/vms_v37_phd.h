#ifndef VMS_V37_PHD_H
#define VMS_V37_PHD_H
#include <stddef.h>
#include <stdint.h>

/* SYSDEFMP.MDL $STRUCT PHD fixed portion (376 bytes).  The P0/P1 base
 * fields carry native page-table addresses supplied by the port; this
 * module never manufactures VAX virtual-address bias values. */
enum {
    PORT_V37_PHD_BYTES=376u,
    PORT_V37_PHD_PRIVMSK=0u,
    PORT_V37_PHD_PHVINDEX=66u,
    PORT_V37_PHD_KSP=116u,
    PORT_V37_PHD_R4=148u,
    PORT_V37_PHD_PC=188u,
    PORT_V37_PHD_P0BR=196u,
    PORT_V37_PHD_P0LRASTL=200u,
    PORT_V37_PHD_P0LR=200u,
    PORT_V37_PHD_ASTLVL=203u,
    PORT_V37_PHD_P1BR=204u,
    PORT_V37_PHD_P1LR=208u,
    PORT_V37_PHD_FREP1VA=48u,
    PORT_V37_PHD_AUTHPRIV=220u,
    PORT_V37_PHD_AUTHPRI=264u,
    PORT_V37_PHD_WSL=376u
};
enum {PORT_V37_PHD_OK=1u,PORT_V37_PHD_BADPARAM=2u};

struct port_v37_phd_seed {
    uint32_t native_ksp,entry_pc,initial_r4;
    uint32_t p0_page_table,p0_limit,p1_page_table,p1_limit;
    uint32_t first_free_p1_va;
    uint8_t priority,ast_level;
};

/* Encode only the fields assigned by PDAT.MAR GENPHD.  Remaining fixed PHD
 * bytes start zeroed like the source BLKB followed by explicit stores. */
uint32_t port_v37_phd_init_genphd(uint8_t *,size_t,
    const struct port_v37_phd_seed *);
#endif
