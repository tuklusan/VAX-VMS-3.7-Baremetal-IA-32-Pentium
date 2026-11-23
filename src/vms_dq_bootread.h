#ifndef VMS_DQ_BOOTREAD_H
#define VMS_DQ_BOOTREAD_H
#include <stddef.h>
#include <stdint.h>
#include "vms_dq_fdt.h"
#include "vms_ini_devicedb.h"
#include "vms_qio_post.h"
/* DRIVER/SRC/DQDRIVER.MAR PREPROCESS (640-671), FDISPATCH (678-709),
 * NORMAL/FUNCXT (790-940) on native ATA. This is a privileged physical
 * block path, not a complete DQ driver or public user $QIO implementation.
 * Source code uses IRP$L_MEDIA at offset 56 to hold the LBN before
 * completion, sharing storage with IOST1 after completion. */
enum {PORT_DQ_BOOTREAD_OK=1u, PORT_DQ_BOOTREAD_BAD=2u,
      PORT_DQ_BOOTREAD_UNBOUND=4u,PORT_DQ_BOOTREAD_MEDIA=6u,
      PORT_DQ_BOOTREAD_FAILED=8u,PORT_DQ_BOOTREAD_DONE=10u,
      PORT_DQ_PHYSIO=0x0100u,PORT_DQ_UCB_MEDIA=156u,
      PORT_DQ_UCB_FEX=134u,PORT_DQ_UCB_BCR=166u,
      PORT_DQ_UCB_FUNC=142u,PORT_DQ_UCB_VALID=0x0800u,
      PORT_DQ_BOOTREAD_LBN=1u,PORT_DQ_BOOTREAD_BYTES=512u};
typedef int (*port_dq_native_read_fn)(void *,uint32_t,uint16_t[256]);
typedef int (*port_dq_native_write_fn)(void *,uint32_t,const uint16_t[256]);
struct port_dq_bootread_request {
    const struct port_dq_fdt *fdt;
    const uint8_t *device_db;
    const struct port_v37_device_result *device_info;
    uint8_t *irp;
    size_t irp_bytes;
    uint16_t *sector;
    size_t sector_bytes;
    uint16_t boot_unit;
    /* Set only by validated native DQ_STARTIO after DDT publication. */
    uint32_t active_ddt_base;
    port_dq_native_read_fn read_sector;
    port_dq_native_write_fn write_sector;
    void *read_context;
};
struct port_dq_bootread_result {
    uint32_t lbn,bytes,irp_status,irp_second;
    uint32_t ucb_media,ucb_io_count;
    uint16_t io_function,unit;
    uint16_t sector_checksum,software_online;
};
uint32_t port_dq_bootread_preflight(const struct port_dq_bootread_request *);
uint32_t port_dq_bootread_decode(const struct port_dq_bootread_request *,
                                uint32_t *lbn,uint16_t *function);
uint32_t port_dq_bootread_commit(const struct port_dq_bootread_request *,
    const uint16_t [256],struct port_dq_bootread_result *);
uint32_t port_dq_bootwrite_commit(const struct port_dq_bootread_request *,
    struct port_dq_bootread_result *);
uint16_t port_dq_bootread_checksum(const uint16_t [256]);
uint32_t port_dq_bootread_execute(const struct port_dq_bootread_request *,
                                 struct port_dq_bootread_result *);
/* A second privileged bootstrap read. The requested LBN is explicit and
 * never obtained from user memory. No hardware writes or public QIO. */
uint32_t port_dq_bootread_execute_physical(const struct port_dq_bootread_request *,
    struct port_dq_bootread_result *,uint32_t requested_lbn);
uint32_t port_dq_bootwrite_execute_physical(const struct port_dq_bootread_request *,
    struct port_dq_bootread_result *,uint32_t requested_lbn);
uint32_t port_dq_bootread_decode_physical(const struct port_dq_bootread_request *,
    uint32_t *lbn,uint16_t *function,uint32_t requested_lbn);
#endif
