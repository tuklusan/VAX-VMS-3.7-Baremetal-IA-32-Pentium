#ifndef VMS_INI_PROCVEC_H
#define VMS_INI_PROCVEC_H
#include <stddef.h>
#include <stdint.h>
/* SYS/SRC/INIT.MAR INI_PSTATE: PCB pointers and sequence vector sharing
 * one 12-byte DYN record. A separate source-derived scheduler must still
 * attach the real process headers and the swapper / null process. */
enum { PORT_INI_PROC_OK=1u,PORT_INI_PROC_BADPARAM=2u,
       PORT_INI_PROC_NOMEM=4u };
struct port_ini_proc_settings {
    uint32_t max_processes;
    uint32_t system_pcb_address, null_pcb_address;
    uint16_t dyn_pcbvec_type; /* supplied from original DYNDEF when ported */
};
struct port_ini_procvec {
    uint8_t *pcb_vector, *sequence_vector;
    uint32_t system_pix,max_pix,allocated_slots,allocated_bytes;
};
uint32_t port_ini_procvec_init(void *pool,size_t capacity,
                 const struct port_ini_proc_settings *settings,
                 struct port_ini_procvec *out);
uint32_t port_ini_procvec_set_pcb(struct port_ini_procvec *,uint32_t,
                                  uint32_t);
uint32_t port_ini_procvec_ptr(const struct port_ini_procvec *p,uint32_t pix);
uint16_t port_ini_procvec_seq(const struct port_ini_procvec *p,uint32_t pix);
#endif
