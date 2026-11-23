#ifndef VMS37_NATIVE_RELOCDRV_H
#define VMS37_NATIVE_RELOCDRV_H
#include <stddef.h>
#include <stdint.h>
#include "vms_v37_dyn_tags.h"
#include "vms_ini_devicedb.h"
#include "vms_dqddt.h"
/* SYS/SRC/RELOCDRV.MAR 61-262 / SHRLIB/SYSMAR.MAR DPT_STORE.
 * An original DPT STORE program executes only against explicitly supplied,
 * bounded native DDB/UCB/CRB/IDB record regions. Unknown driver structures,
 * raw pointers and unavailable RB730/MMIO are never dereferenced. */
enum { PORT_RELOCDRV_OK=1u,PORT_RELOCDRV_BAD=2u,
       PORT_RELOCDRV_UNBOUND=4u,PORT_RELOCDRV_OVERFLOW=6u,
       PORT_RELOCDRV_REGION_COUNT=4u,PORT_RELOCDRV_OP_CAP=32u,
       PORT_RELOCDRV_DQ_INIT_COUNT=9u,PORT_RELOCDRV_DQ_INIT_BYTES=47u,
       PORT_RELOCDRV_DQ_UCB_BYTES=PORT_V37_BOOT_UCB_SIZE,
       PORT_RELOCDRV_DQ_DDB_BYTES=PORT_V37_DDB_SIZE,
       PORT_RELOCDRV_DQ_CURDA=206u,PORT_RELOCDRV_DQ_CHAR=0x1c454008u};
enum { PORT_RELOCDRV_OP_BYTE=0u,PORT_RELOCDRV_OP_WORD=1u,
       PORT_RELOCDRV_OP_DISPLACED=2u,PORT_RELOCDRV_OP_LONG=3u,
       PORT_RELOCDRV_OP_FIELD=4u,PORT_RELOCDRV_OP_INDIRECT=0x80u };
struct port_relocdrv_region {
    uint8_t *bytes;size_t bytes_size;uint32_t pc_base;uint8_t structure_type;
};
struct port_relocdrv_context {
    struct port_relocdrv_region region[PORT_RELOCDRV_REGION_COUNT];
    const uint8_t *readonly_bytes;size_t readonly_size;uint32_t readonly_base;
    uint32_t dpt_base;
};
struct port_relocdrv_op {
    uint8_t type,offset,opcode,pos,bits;
    uint32_t raw_value;
    size_t next;
};
struct port_relocdrv_dq {
    uint8_t ddb[PORT_RELOCDRV_DQ_DDB_BYTES] __attribute__((aligned(4)));
    uint8_t ucb[PORT_RELOCDRV_DQ_UCB_BYTES] __attribute__((aligned(4)));
    uint32_t source_ddb,source_ucb;
    uint32_t applied,ready,driver_bound;
};
uint16_t port_relocdrv_read16(const uint8_t *);
uint32_t port_relocdrv_read32(const uint8_t *);
void port_relocdrv_write(uint8_t *,unsigned,uint32_t);
uint32_t port_relocdrv_region(const struct port_relocdrv_context *,
                             uint8_t,unsigned,uint8_t **);
uint32_t port_relocdrv_decode(const uint8_t *,size_t,size_t,
                             struct port_relocdrv_op *);
uint32_t port_relocdrv_fetch(const struct port_relocdrv_context *,
                            const struct port_relocdrv_op *,uint32_t *);
uint32_t port_relocdrv_field(uint8_t *,uint32_t,unsigned,unsigned,int);
uint32_t port_relocdrv_step(const struct port_relocdrv_context *,
    const struct port_relocdrv_op *,int);
uint32_t port_relocdrv_walk(const struct port_relocdrv_context *,
    const uint8_t *,size_t,int,uint32_t *);
uint32_t port_relocdrv_isolated(const struct port_relocdrv_context *,
    const uint8_t *,size_t);
uint32_t port_relocdrv_apply(const struct port_relocdrv_context *,
    const uint8_t *,size_t,uint32_t *);
uint32_t port_relocdrv_dq_table(uint8_t *,size_t,size_t *);
uint32_t port_relocdrv_dq_guard(const uint8_t *,size_t,
                               const struct port_v37_device_result *);
uint32_t port_relocdrv_dq_footprint(const struct port_relocdrv_dq *,
    const uint8_t *,size_t);
uint32_t port_relocdrv_dq_check(const struct port_relocdrv_dq *);
uint32_t port_relocdrv_dq_init(struct port_relocdrv_dq *,
    const uint8_t *,size_t,const struct port_v37_device_result *,
    const struct port_dqddt_stage *,const struct port_dq_fdt *);
#endif
