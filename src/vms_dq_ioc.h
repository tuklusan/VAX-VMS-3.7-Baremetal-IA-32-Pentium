#ifndef VMS37_NATIVE_DQ_IOC_H
#define VMS37_NATIVE_DQ_IOC_H
#include <stdint.h>
#include <stddef.h>
/* DRIVER/SRC/DQDRIVER.MAR DQ_STARTIO/PREPROCESS/FDISPATCH/FUNCXT
 * and SYS/SRC/IOCIOPOST.MAR I/O packet queue and completion ordering.
 * Bounded, private supervisor physical READPBLK queue. Resident SYS.EXE
 * can drive its single physical request through the attached native CRB/IRQ
 * callback. No public $QIO, process AST, ACP or running-IRP cancellation. */
#define PORT_DQ_IOC_DEPTH 8u
enum { PORT_DQ_IOC_OK=1u,PORT_DQ_IOC_BAD=2u,
       PORT_DQ_IOC_FULL=4u,PORT_DQ_IOC_BUSY=6u,
       PORT_DQ_IOC_NOTFOUND=8u,PORT_DQ_IOC_IO=10u,
       PORT_DQ_IOC_CORRUPT=12u };
enum { PORT_DQ_IOC_EMPTY=0u,PORT_DQ_IOC_PREPARED=1u,
       PORT_DQ_IOC_QUEUED=2u,PORT_DQ_IOC_ACTIVE=3u,
       PORT_DQ_IOC_FINISHED=4u,PORT_DQ_IOC_FAILED=5u };
typedef uint32_t (*port_dq_ioc_sector_fn)(void *,uint32_t,uint8_t [512]);
typedef void (*port_dq_ioc_async_done_fn)(void *,uint32_t);
typedef int (*port_dq_ioc_async_start_fn)(void *,uint32_t,uint16_t [256],
    port_dq_ioc_async_done_fn,void *);
struct port_dq_ioc_packet {
    uint32_t tag,lbn,order;
    uint8_t *destination;
    uint8_t state;
    uint8_t retries_remaining;
    uint8_t retry_limit;
    uint8_t attempts;
    uint8_t priority;
    uint8_t reserved[3];
    uint8_t sector[512];
};
struct port_dq_ioc_queue {
    struct port_dq_ioc_packet packets[PORT_DQ_IOC_DEPTH];
    port_dq_ioc_sector_fn read_sector;
    void *context;
    uint32_t order,submitted,completed,failed,reads,active;
    uint32_t retried,posted;
};
uint32_t port_dq_ioc_init(struct port_dq_ioc_queue *,
                           port_dq_ioc_sector_fn,void *);
uint32_t port_dq_ioc_validate(const struct port_dq_ioc_queue *);
uint32_t port_dq_ioc_prepare(struct port_dq_ioc_queue *,
                           uint32_t,uint32_t,uint8_t *,size_t);
uint32_t port_dq_ioc_set_priority(struct port_dq_ioc_queue *,uint32_t,uint8_t);
uint32_t port_dq_ioc_enqueue(struct port_dq_ioc_queue *,uint32_t);
uint32_t port_dq_ioc_start(struct port_dq_ioc_queue *,uint32_t *);
/* Start one priority-selected request against an asynchronous sector engine.
 * The callback only changes the active packet state; post remains deferred. */
uint32_t port_dq_ioc_start_async(struct port_dq_ioc_queue *,
    port_dq_ioc_async_start_fn,uint32_t *);
uint32_t port_dq_ioc_async_complete(struct port_dq_ioc_queue *,uint32_t);
uint32_t port_dq_ioc_transfer(struct port_dq_ioc_queue *);
uint32_t port_dq_ioc_complete(struct port_dq_ioc_queue *,uint32_t *);
uint32_t port_dq_ioc_cancel(struct port_dq_ioc_queue *,uint32_t);
/* DQDRIVER.MAR RETRYERR/RESETDRIVE and FUNCXT, IOCIOPOST.MAR
 * bounded private physical I/O lifecycle. The RB730 recalibrate command,
 * interrupt acknowledgement and process AST are NOT emulated here. */
#define PORT_DQ_IOC_MAX_RETRIES 15u
struct port_dq_ioc_iosb { uint32_t tag; uint16_t status, bytes; };
struct port_dq_ioc_report { uint32_t processed, succeeded, failed, reads; };
uint32_t port_dq_ioc_set_retry(struct port_dq_ioc_queue *,uint32_t,uint8_t);
uint32_t port_dq_ioc_attempt(struct port_dq_ioc_queue *);
uint32_t port_dq_ioc_recover(struct port_dq_ioc_queue *);
uint32_t port_dq_ioc_retry(struct port_dq_ioc_queue *);
uint32_t port_dq_ioc_iosb(const struct port_dq_ioc_queue *,struct port_dq_ioc_iosb *);
uint32_t port_dq_ioc_post(struct port_dq_ioc_queue *,struct port_dq_ioc_iosb *);
uint32_t port_dq_ioc_service(struct port_dq_ioc_queue *,struct port_dq_ioc_iosb *);
uint32_t port_dq_ioc_run(struct port_dq_ioc_queue *,struct port_dq_ioc_report *);
#endif
