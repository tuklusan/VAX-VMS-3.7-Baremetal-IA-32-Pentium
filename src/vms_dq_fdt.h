#ifndef VMS_DQ_FDT_H
#define VMS_DQ_FDT_H
#include <stddef.h>
#include <stdint.h>
/* DRIVER/SRC/DQDRIVER.MAR 496-590: the original DQ FUNCTAB masks.
 * These are the original VMS 3.7 virtual-function numbers, not PC opcodes.
 * No ACP, DQSTARTIO, CRB, or actual device DDT is linked by these masks.
 * SHRLIB/SYSMAR.MAR FUNCTAB macro emits pairs of longword masks.
 * The separately loaded native DQ driver remains an unported dependency. */
enum { PORT_DQ_FDT_ROWS=12u, PORT_DQ_FDT_BYTES=96u,
  PORT_DQ_FDT_LEGAL=0u, PORT_DQ_FDT_BUFFERED=1u,
  PORT_DQ_FDT_READ=2u,PORT_DQ_FDT_WRITE=3u,
  PORT_DQ_FDT_ACCESS=4u,PORT_DQ_FDT_DEACCESS=5u,
  PORT_DQ_FDT_MODIFY=6u,PORT_DQ_FDT_MOUNT=7u,
  PORT_DQ_FDT_ZERO=8u,PORT_DQ_FDT_ONE=9u,
  PORT_DQ_FDT_SENSE=10u,PORT_DQ_FDT_SET=11u,
  PORT_DQ_FDT_OK=1u,PORT_DQ_FDT_BAD=2u,
  PORT_DQ_FDT_ILLEGAL=4u,PORT_DQ_FDT_UNBOUND=6u,
  PORT_DQ_FDT_NO_ACTION=0xffu };
/* Functions whose bit numbers are in VMSLIB/SRC/STARDEFFL.MDL 337-428. */
enum {
  PORT_DQ_NOP=0,PORT_DQ_UNLOAD=1,PORT_DQ_SEEK=2,
  PORT_DQ_RECAL=3,PORT_DQ_DRVCLR=4,PORT_DQ_PACKACK=8,
  PORT_DQ_WRITECHECK=10,PORT_DQ_WRITEPBLK=11,
  PORT_DQ_READPBLK=12,PORT_DQ_WRITEHEAD=13,PORT_DQ_READHEAD=14,
  PORT_DQ_AVAILABLE=17,PORT_DQ_SETCHAR=26,PORT_DQ_SENSECHAR=27,
  PORT_DQ_WRITELBLK=32,PORT_DQ_READLBLK=33,
  PORT_DQ_SETMODE=35,PORT_DQ_SENSEMODE=39,
  PORT_DQ_WRITEVBLK=48,PORT_DQ_READVBLK=49,
  PORT_DQ_ACCESS=50,PORT_DQ_CREATE=51,PORT_DQ_DEACCESS=52,
  PORT_DQ_DELETE=53,PORT_DQ_MODIFY=54,
  PORT_DQ_ACPCONTROL=56,PORT_DQ_MOUNT=57
};
struct port_dq_fdt {
  uint64_t mask[PORT_DQ_FDT_ROWS];
};
struct port_dq_fdt_decision {
  uint16_t full_function;
  uint8_t number,buffered,action_class,driver_bound;
};
#define PORT_DQ_BIT(n) (UINT64_C(1)<<(n))
uint64_t port_dq_fdt_legal(void);
uint64_t port_dq_fdt_buffered(void);
uint64_t port_dq_fdt_read(void);
uint64_t port_dq_fdt_write(void);
uint64_t port_dq_fdt_access(void);
uint64_t port_dq_fdt_deaccess(void);
uint64_t port_dq_fdt_modify(void);
uint64_t port_dq_fdt_mount(void);
uint64_t port_dq_fdt_zero(void);
uint64_t port_dq_fdt_one(void);
uint64_t port_dq_fdt_sense(void);
uint64_t port_dq_fdt_set(void);
uint32_t port_dq_fdt_build(struct port_dq_fdt *,size_t);
uint32_t port_dq_fdt_validate(const struct port_dq_fdt *,size_t);
uint32_t port_dq_fdt_classify(const struct port_dq_fdt *,size_t,uint16_t,
                             struct port_dq_fdt_decision *);
uint32_t port_dq_fdt_dispatch(const struct port_dq_fdt *,size_t,uint16_t);
#endif
