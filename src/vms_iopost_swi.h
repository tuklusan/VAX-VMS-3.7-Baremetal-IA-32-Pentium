#ifndef VMS37_IOPOST_SWI_H
#define VMS37_IOPOST_SWI_H
#include <stdint.h>
/* SCBVECTOR.MAR 109-113 maps software IPL$_IOPOST (4) at VAX
 * vector byte offset 0x90. Pentium native IDT 0x90 is CPL0-only;
 * it is NOT a 8259 PIC IRQ and must never send a PIC EOI. */
enum {PORT_IOPOST_SW_VECTOR=0x90u,PORT_IOPOST_SW_OK=1u,
      PORT_IOPOST_SW_BAD=2u,PORT_IOPOST_SW_UNBOUND=4u,
      PORT_IOPOST_SW_BUSY=6u,PORT_IOPOST_SW_CORRUPT=8u};
typedef uint32_t (*port_iopost_swi_action)(void);
struct port_iopost_swi {
    port_iopost_swi_action action;
    uint32_t guest_entry,lower,upper;
    uint32_t calls,attempts,last_status;
    uint8_t in_service,armed;
};
void port_iopost_swi_init(struct port_iopost_swi *);
uint32_t port_iopost_swi_bind(struct port_iopost_swi *,
    port_iopost_swi_action,uint32_t,uint32_t,uint32_t);
uint32_t port_iopost_swi_dispatch(struct port_iopost_swi *,unsigned);
uint32_t port_iopost_swi_check(const struct port_iopost_swi *,
    uint32_t,uint32_t);
#endif
