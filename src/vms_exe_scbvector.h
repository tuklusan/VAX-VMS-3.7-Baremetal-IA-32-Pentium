#ifndef VMS_EXE_SCBVECTOR_H
#define VMS_EXE_SCBVECTOR_H
#include <stdint.h>
/* SYS/SRC/SCBVECTOR.MAR. VAX SCB offsets remain stable across the PC port. */
enum {PORT_SCB_ENTRIES=128,PORT_SCB_NO_PC_VECTOR=255u};
struct port_exe_scb_slot {
    const char *original_handler;
    uint16_t vax_byte_offset;
    uint8_t interrupt_stack;
    uint8_t pc_idt_vector;
    uint8_t unused;
};
/* These entries name service handlers; this does not install an IDT. */
void port_exe_scb_init(struct port_exe_scb_slot slots[PORT_SCB_ENTRIES]);
const struct port_exe_scb_slot *port_exe_scb_get(const struct port_exe_scb_slot slots[PORT_SCB_ENTRIES],uint16_t original_offset);
#endif
