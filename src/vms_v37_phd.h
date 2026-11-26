#ifndef VMS_V37_PHD_H
#define VMS_V37_PHD_H
#include <stddef.h>
#include <stdint.h>

/* SYSDEFMP.MDL $STRUCT PHD fixed portion (376 bytes).  The P0/P1 base
 * fields carry native page-table addresses supplied by the port; this
 * module never manufactures VAX virtual-address bias values. */
enum {
    PORT_V37_PHD_BYTES=376u,
    /* SYSDEFMP.MDL PHD$L_PCB precedes the embedded hardware context. */
    PORT_V37_PHD_PCB=112u,
    PORT_V37_PHD_WSLIST=8u,PORT_V37_PHD_WSAUTH=10u,
    PORT_V37_PHD_WSAUTHEXT=20u,PORT_V37_PHD_WSEXTENT=22u,
    PORT_V37_PHD_WSQUOTA=24u,PORT_V37_PHD_DFWSCNT=26u,
    PORT_V37_PHD_CPUTIM=56u,PORT_V37_PHD_ASTLM=64u,
    PORT_V37_PHD_CPULIM=88u,
    PORT_V37_PHD_ASTLVL_DISABLED=4u,
    PORT_V37_PHD_PRIVMSK=0u,
    PORT_V37_PHD_PRIVMSK_BYTES=8u,
    /* PRVDEF positions tested by SHRLIB/SYSMAR.MAR IFPRIV. */
    PORT_V37_PRIV_LOG_IO=7u,PORT_V37_PRIV_PHY_IO=22u,
    PORT_V37_PHD_PHVINDEX=66u,
    PORT_V37_PHD_KSP=116u,
    PORT_V37_PHD_R0=132u,
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
    /* SYSDEFMP.MDL PHD$L_DIOCNT/BIOCNT follow SWAPSIZE. */
    PORT_V37_PHD_DIOCNT=80u,PORT_V37_PHD_BIOCNT=84u,
    PORT_V37_PHD_WSL=376u
};
enum {PORT_V37_PHD_OK=1u,PORT_V37_PHD_BADPARAM=2u};

struct port_v37_phd_seed {
    uint32_t native_ksp,entry_pc,initial_r4;
    uint32_t p0_page_table,p0_limit,p1_page_table,p1_limit;
    uint32_t first_free_p1_va;
    /* IA-32 address-space root is port-side scheduler state; it does not
     * replace the source P0/P1 base and limit fields in the PHD. */
    uint32_t native_cr3;
    uint8_t priority,ast_level;
};

/* PROCSTRT.MAR SETQUOTAS working-set normalization inputs. Quota values are
 * longwords in the PQB and are narrowed to the source word-sized PHD fields. */
struct port_v37_phd_ws_quotas {
    uint32_t physical_pages,free_page_limit,max_working_set;
    uint32_t extent,quota,default_size;
};
struct port_v37_phd_process_quotas {
    uint32_t cpu_limit,ast_limit;
    uint64_t privileges;
    uint8_t base_priority;
};
enum {PORT_V37_PHD_QUOTA_OK=1u,PORT_V37_PHD_QUOTA_BADPARAM=2u};

/* Encode only the fields assigned by PDAT.MAR GENPHD.  Remaining fixed PHD
 * bytes start zeroed like the source BLKB followed by explicit stores. */
uint32_t port_v37_phd_init_genphd(uint8_t *,size_t,
    const struct port_v37_phd_seed *);
/* PROCSTRT.MAR 261-287 available-page cap, extent/quota/default ordering,
 * and WSLIST-relative PHD limits. All five stores occur transactionally. */
uint32_t port_v37_phd_apply_ws_quotas(uint8_t *,size_t,
    const struct port_v37_phd_ws_quotas *);
/* PROCSTRT.MAR CPU/AST limits and PQB privilege masks in PHD/authorized PHD. */
uint32_t port_v37_phd_apply_process_quotas(uint8_t *,size_t,
    const struct port_v37_phd_process_quotas *);
#endif
