#ifndef VMS_DQ_CRB_H
#define VMS_DQ_CRB_H
#include <stdint.h>
/* Native adapter for the SYSDEF.MDL CRB and its two 32-byte VECs.
 * VAX instruction dispatch words remain zero; IA-32 IRQ dispatch is a
 * separately typed callback consumed by the real IDT path. */
enum { PORT_DQ_CRB_STANDARD_BYTES=72u,PORT_DQ_CRB_BYTES=108u,
       PORT_DQ_CRB_IRQ14_VECTOR=46u,
       PORT_DQ_CRB_OK=1u,PORT_DQ_CRB_BAD=2u };
typedef void (*port_dq_crb_handler)(void);
struct port_dq_crb {
    uint8_t source[PORT_DQ_CRB_BYTES];
    port_dq_crb_handler irq_handler;
    uint32_t vector,interrupts;
    uint8_t ready,reserved[3];
} __attribute__((aligned(4)));
uint32_t port_dq_crb_init(struct port_dq_crb *,port_dq_crb_handler,uint32_t);
uint32_t port_dq_crb_validate(const struct port_dq_crb *);
uint32_t port_dq_crb_interrupt(struct port_dq_crb *,uint32_t);
#endif
