#ifndef VMS37_SYSQIO_ARGS_H
#define VMS37_SYSQIO_ARGS_H
#include <stddef.h>
#include <stdint.h>
#include "vms_dq_fdt.h"
#include "vms_sysqio_ef.h"
#include "vms_sysqio_iosb.h"
#include "vms_qio_irp_pool.h"
#include "vms_ini_packet_queues.h"
#include "vms_v37_ccb.h"
#include "vms_exe_access.h"

/* Native decoder for the fixed SYS$QIO/EXE$QIO argument list. The argument
 * list retains the VAX AP layout: count longword followed by 12 longwords. */
enum { PORT_V37_SYSQIO_REQUIRED_ARGS=12u,
       PORT_V37_SYSQIO_LIST_BYTES=52u,
       PORT_V37_SYSQIO_ARGS_OK=1u,
       PORT_V37_SYSQIO_ARGS_BAD=2u };

struct port_v37_sysqio_args {
    uint32_t event_flag,channel;
    uint16_t function;
    uint16_t reserved;
    uint32_t iosb,ast_address,ast_parameter;
    uint32_t parameter[6];
};

uint32_t port_v37_sysqio_decode_args(const void *argument_list,
    size_t accessible_bytes,struct port_v37_sysqio_args *decoded);

typedef uint32_t (*port_v37_sysqio_volume_check_fn)(void *,uint8_t,
    uint16_t,uint32_t,uint32_t);
enum { PORT_V37_SYSQIO_PRECHECK_OK=1u,
       PORT_V37_SYSQIO_PRECHECK_BAD=2u,
       PORT_V37_SYSQIO_PRECHECK_NOPRIV=4u,
       PORT_V37_SYSQIO_PRECHECK_ILLFUNC=6u,
       PORT_V37_SYSQIO_PRECHECK_OFFLINE=8u };
struct port_v37_sysqio_device {
    const struct port_dq_fdt *fdt;
    size_t fdt_bytes;
    uint8_t online;
    uint8_t reserved[3];
    uint32_t characteristics,owner_uic;
    uint16_t volume_protection,reserved2;
    port_v37_sysqio_volume_check_fn check_volume;
    void *volume_context;
};
struct port_v37_sysqio_dispatch {
    uint16_t full_function;
    uint8_t function_code,buffered,action_class,physical_io;
};

/* SYSQIOREQ.MAR channel access, FDT legality and offline-device checks.
 * These result values are internal and are not VMS condition codes. */
uint32_t port_v37_sysqio_preflight(const struct port_v37_sysqio_args *,
    uint8_t caller_mode,uint8_t ccb_access_limit,
    const struct port_v37_sysqio_device *,
    struct port_v37_sysqio_dispatch *);

enum { PORT_V37_SYSQIO_BEGIN_OK=1u,PORT_V37_SYSQIO_BEGIN_BAD=2u,
       PORT_V37_SYSQIO_BEGIN_IVCHAN=4u,
       PORT_V37_SYSQIO_BEGIN_NOPRIV=6u,
       PORT_V37_SYSQIO_BEGIN_ILLFUNC=8u,
       PORT_V37_SYSQIO_BEGIN_OFFLINE=10u,
       PORT_V37_SYSQIO_BEGIN_ACCVIO=12u,
       PORT_V37_SYSQIO_BEGIN_EFERR=14u,
       PORT_V37_SYSQIO_BEGIN_EXQUOTA=16u,
       PORT_V37_SYSQIO_BEGIN_WAIT=18u };
struct port_v37_sysqio_resolved_channel {
    const struct port_v37_sysqio_device *device;
    void *ucb;
    uint32_t window;
    uint8_t ccb_access_limit;
    uint8_t reserved[3];
};
struct port_v37_sysqio_process {
    uint32_t pid;
    uint32_t arb_va;
    uint32_t phd_va;
    uint8_t access_mode;
    uint8_t has_phy_io,has_log_io,reserved;
    uint32_t local_event_flags[2];
    int16_t buffered_io_count,direct_io_count;
    int16_t ast_count;
    uint8_t *source_pcb;
    size_t source_pcb_bytes;
    uint8_t *source_phd;
    size_t source_phd_bytes;
    /* Per-process CTL$GL_CCBBASE/CTL$GW_CHINDX state. The table bytes are
     * owned by the process address-space/CTL allocator supplied by INIT. */
    uint8_t *ccb_base;
    size_t ccb_preceding_bytes;
    uint16_t ccb_channel_count,ccb_high_water;
};
enum { PORT_V37_SYSQIO_PCB_OK=1u,PORT_V37_SYSQIO_PCB_BAD=2u,
       PORT_V37_SYSQIO_PCB_DIRECT=0u,PORT_V37_SYSQIO_PCB_BUFFERED=1u };
uint32_t port_v37_sysqio_process_bind_pcb(uint8_t *,size_t,uint8_t *,size_t,
    uint32_t,uint8_t,struct port_v37_sysqio_process *);
uint32_t port_v37_sysqio_process_bind_channels(
    struct port_v37_sysqio_process *,uint8_t *,size_t,uint16_t);
int16_t port_v37_sysqio_process_io_count(
    const struct port_v37_sysqio_process *,uint8_t);
void port_v37_sysqio_process_set_io_count(
    struct port_v37_sysqio_process *,uint8_t,int16_t);
int16_t port_v37_sysqio_process_ast_count(
    const struct port_v37_sysqio_process *);
void port_v37_sysqio_process_set_ast_count(
    struct port_v37_sysqio_process *,int16_t);
typedef int (*port_v37_sysqio_single_quota_fn)(void *,uint32_t,uint8_t);
struct port_v37_sysqio_begin_ops {
    void *context;
    port_v37_sysqio_clref_fn clear_global_efn;
    /* The source channel table is addressed backward from CTL$GL_CCBBASE.
     * A live UCB/device resolver supplies the native DDB/FDT view. */
    uint8_t *ccb_base;
    size_t ccb_preceding_bytes;
    uint16_t ccb_maximum_channel_offset;
    int (*lookup_channel)(void *,uint32_t,uint32_t,
        struct port_v37_sysqio_resolved_channel *);
    int (*lookup_ucb)(void *,uint32_t,uint32_t,
        struct port_v37_sysqio_resolved_channel *);
    /* SCH$WAIT for RSN$_ASTWAIT and return after the CCB access/deaccess
     * pending bit is cleared. Required only when resolved window bit 0 is set. */
    void (*wait_channel)(void *,uint32_t,uint32_t,
        struct port_v37_sysqio_resolved_channel *);
    void (*post_event)(void *,uint32_t,uint8_t,uint32_t);
    port_v37_sysqio_single_quota_fn single_io_quota;
    struct port_v37_sysqio_memory user_memory;
};
struct port_v37_sysqio_prepared {
    struct port_v37_sysqio_args args;
    struct port_v37_sysqio_dispatch dispatch;
    struct port_v37_sysqio_resolved_channel channel;
};

/* Source SYSQIOREQ SUCCES constructs the device-independent standard IRP.
 * This builder only consumes an already reserved packet; allocation,
 * channel IOC accounting, device access checks and FDT invocation remain
 * caller responsibilities. */
enum { PORT_V37_SYSQIO_IRP_BUILD_OK=1u,
       PORT_V37_SYSQIO_IRP_BUILD_BAD=2u,
       PORT_V37_SYSQIO_IRP_STANDARD_BYTES=136u };
struct port_v37_sysqio_irp_spec {
    /* channel_index is the positive CCB byte offset (16-byte units). */
    uint32_t packet_va,ucb_va,arb_va,window_va,channel_index;
    uint8_t base_priority;
    uint8_t reserved[3];
};
uint32_t port_v37_sysqio_build_irp(const struct port_v37_sysqio_prepared *,
    struct port_v37_sysqio_process *,const struct port_v37_sysqio_irp_spec *,
    uint8_t *packet,size_t packet_bytes);
/* Allocate and construct one source-layout IRP. On any post-allocation
 * failure, return the packet to the same INIT lookaside queue. */
enum { PORT_V37_SYSQIO_RESERVE_OK=1u,PORT_V37_SYSQIO_RESERVE_BAD=2u,
       PORT_V37_SYSQIO_RESERVE_EMPTY=4u,PORT_V37_SYSQIO_RESERVE_ASTQUOTA=6u,
       PORT_V37_SYSQIO_RESERVE_POOL=8u };
uint32_t port_v37_sysqio_reserve_irp(
    const struct port_v37_sysqio_prepared *,struct port_v37_sysqio_process *,
    const struct port_v37_sysqio_irp_spec *,struct port_ini_packet_list *,
    const struct port_v37_irp_pool_sync *,uint8_t **,uint32_t *,
    uint32_t *service_status);
enum { PORT_V37_SYSQIO_AST_OK=1u,PORT_V37_SYSQIO_AST_BAD=2u,
       PORT_V37_SYSQIO_AST_EXQUOTA=4u };
uint32_t port_v37_sysqio_check_ast_quota(
    const struct port_v37_sysqio_prepared *,
    const struct port_v37_sysqio_process *,uint32_t *service_status);

enum { PORT_V37_SYSQIO_ACCESS_PHYSICAL=1u,
       PORT_V37_SYSQIO_ACCESS_LOGICAL=2u,
       PORT_V37_SYSQIO_DEVICE_OK=1u,PORT_V37_SYSQIO_DEVICE_BAD=2u,
       PORT_V37_SYSQIO_DEVICE_NOPRIV=4u };
uint32_t port_v37_sysqio_device_access(uint16_t function,
    uint32_t device_characteristics,uint8_t has_phy_io,uint8_t has_log_io,
    uint16_t volume_protection,uint32_t owner_uic,uint32_t arb_va,
    port_v37_sysqio_volume_check_fn check_volume,void *context,
    uint8_t *physical_io,uint32_t *service_status);

/* SYSQIOREQ ACCESS: normalize WRITEVBLK for the boot-time device states. */
enum { PORT_V37_SYSQIO_ACCESS_OK=1u,PORT_V37_SYSQIO_ACCESS_BAD=2u,
       PORT_V37_SYSQIO_ACCESS_NOPRIV=4u };
enum { PORT_V37_DEVCHAR_SPL=1u<<6,PORT_V37_DEVCHAR_FOD=1u<<14,
       PORT_V37_DEVCHAR_SHR=1u<<16,PORT_V37_DEVCHAR_MNT=1u<<19,
       PORT_V37_DEVCHAR_FOR=1u<<24 };
uint32_t port_v37_sysqio_normalize_write(uint16_t function,
    uint32_t device_characteristics,uint32_t window_address,
    uint16_t *normalized_function,uint8_t *ast_quota_check,
    uint32_t *service_status);

/* Source-ordered EXE$QIO path through IOSB and I/O quota reservation. It
 * prepares a request for a later driver dispatcher; no IRP is allocated and
 * no hardware I/O is initiated here. */
uint32_t port_v37_sysqio_begin(const void *,size_t,
    struct port_v37_sysqio_process *,const struct port_v37_sysqio_begin_ops *,
    struct port_v37_sysqio_prepared *,uint32_t *service_status);

/* Source-ordered SYSQIOREQ front end through standard IRP and AST quota
 * reservation. Failed reservation restores the per-process I/O charge and
 * posts the service error; driver dispatch and submission remain outstanding. */
enum { PORT_V37_SYSQIO_PREPARE_OK=1u,PORT_V37_SYSQIO_PREPARE_BEGIN=2u,
       PORT_V37_SYSQIO_PREPARE_RESERVE=4u,
       PORT_V37_SYSQIO_PREPARE_WAIT=6u,
       PORT_V37_SYSQIO_PREPARE_ACCESS=8u };
uint32_t port_v37_sysqio_prepare_irp(const void *,size_t,
    struct port_v37_sysqio_process *,const struct port_v37_sysqio_begin_ops *,
    const struct port_v37_sysqio_irp_spec *,struct port_ini_packet_list *,
    const struct port_v37_irp_pool_sync *,struct port_v37_sysqio_prepared *,
    uint8_t **,uint32_t *,uint32_t *service_status);
/* Public-entry adapter: copy the fixed VMS AP list from the current
 * caller's translated address space before entering SYSQIOREQ processing. */
uint32_t port_v37_sysqio_prepare_user_irp(
    const struct port_exe_access *,uint32_t,
    struct port_v37_sysqio_process *,const struct port_v37_sysqio_begin_ops *,
    const struct port_v37_sysqio_irp_spec *,struct port_ini_packet_list *,
    const struct port_v37_irp_pool_sync *,struct port_v37_sysqio_prepared *,
    uint8_t **,uint32_t *,uint32_t *service_status);
#endif
