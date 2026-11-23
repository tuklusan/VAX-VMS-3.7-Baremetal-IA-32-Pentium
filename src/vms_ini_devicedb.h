#ifndef PORT_V37_INIT_DEVICEDB_H
#define PORT_V37_INIT_DEVICEDB_H
#include <stddef.h>
#include <stdint.h>
#include "vms_v37_dyn_tags.h"
/* The prelinked DEVICEDAT.MAR device database, in native IA-32 memory.
 * Source DEVICEDAT.MAR 129-254, 354-475; SYSDEF.MDL DDB/UCB layouts.
 * BOOT/OPA/MBA DDT/CRB and unverified STARLET fields remain zero.
 * M122 binds the software-only NLA DDT/FDT; it is not a public $QIO.
 * The native DQA CRB may be attached while DQA's DDT remains unbound. */
enum {
    PORT_V37_DDB_SIZE=52u, PORT_V37_UCB_SIZE=132u,
    PORT_V37_BOOT_UCB_SIZE=252u, PORT_V37_OPA_UCB_SIZE=320u,
    PORT_V37_DEVICE_DDB_COUNT=4u, PORT_V37_DEVICE_UCB_COUNT=5u,
    PORT_V37_DDB_BOOT_OFF=0u,
    PORT_V37_DDB_OPA_OFF=52u,
    PORT_V37_DDB_MB_OFF=104u,
    PORT_V37_DDB_NL_OFF=156u,
    PORT_V37_UCB_BOOT_OFF=208u,
    PORT_V37_UCB_OPA_OFF=460u,
    PORT_V37_UCB_MB1_OFF=780u,
    PORT_V37_UCB_MB2_OFF=912u,
    PORT_V37_UCB_NL_OFF=1044u,
    PORT_V37_DEVICE_BYTES=1176u,
    PORT_V37_DEV_OK=1u, PORT_V37_DEV_BAD=2u,
    PORT_V37_UCB_ONLINE=16u
};
struct port_v37_device_result {
    uint32_t list_head;
    uint32_t ddb[PORT_V37_DEVICE_DDB_COUNT];
    uint32_t ucb[PORT_V37_DEVICE_UCB_COUNT];
    uint32_t bytes;
    uint32_t incomplete_driver_mask;
    uint32_t bound_nl_ddt; /* M122: actual native source NL$DDT table, if bound */
    uint32_t boot_name_ready; /* M127 source FIN_DATABASE DQA name only; DDT remains zero */
    uint32_t bound_dqa_crb; /* Native IRQ14 CRB; never implies a bound disk DDT */
};
void port_v37_write16(uint8_t *,uint16_t);
void port_v37_write32(uint8_t *,uint32_t);
uint16_t port_v37_read16(const uint8_t *);
uint32_t port_v37_read32(const uint8_t *);
void port_v37_ddb_base(uint8_t *,uint32_t,uint32_t,const char *,unsigned,const char *,unsigned);
void port_v37_ucb_base(uint8_t *,uint32_t,uint32_t,uint32_t);
void port_v37_ddb_boot(uint8_t *,uint32_t,uint32_t);
void port_v37_ddb_opa(uint8_t *,uint32_t,uint32_t);
void port_v37_ddb_mb(uint8_t *,uint32_t,uint32_t);
void port_v37_ddb_nl(uint8_t *,uint32_t);
void port_v37_ucb_boot(uint8_t *,uint32_t,uint32_t);
void port_v37_ucb_opa(uint8_t *,uint32_t,uint32_t);
void port_v37_ucb_mb1(uint8_t *,uint32_t,uint32_t,uint32_t);
void port_v37_ucb_mb2(uint8_t *,uint32_t,uint32_t);
void port_v37_ucb_nl(uint8_t *,uint32_t,uint32_t);
uint32_t port_v37_devices_prelink(uint8_t *,size_t,uint32_t,struct port_v37_device_result *);
uint32_t port_v37_devices_check(const uint8_t *,size_t,uint32_t,
                                const struct port_v37_device_result *);
#endif
