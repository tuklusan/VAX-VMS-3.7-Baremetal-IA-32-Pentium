#include "vms_iopost_end.h"

static uint32_t queue_ast(struct port_v37_iopost_sched_binding *binding,
    uint32_t pid,uint32_t expected_pcb,uint32_t irp_va){
    if(!binding||!binding->scheduler||!binding->process_vector||!pid||
       (pid&UINT32_C(0x80000000))||!irp_va||(irp_va&3u))
        return PORT_V37_IOPOST_CORRUPT;
    const uint32_t pcb_va=port_ini_procvec_ptr(binding->process_vector,
                                                pid&UINT16_MAX);
    if(!pcb_va||(expected_pcb&&expected_pcb!=pcb_va))
        return PORT_V37_IOPOST_CORRUPT;
    const uint32_t status=port_v37_sched_queue_ast_rse(binding->scheduler,
        pcb_va,irp_va,0u,binding->priority_increment_class);
    return status==PORT_V37_SCHED_OK?PORT_V37_IOPOST_OK:
                                      PORT_V37_IOPOST_CORRUPT;
}

/* DIRPOST's process PID and retained IRP address resolve to the current PCB
 * vector slot; QAST validates that the IRP still carries the same full PID. */
uint32_t port_v37_iopost_sched_queue_ast(void *context,uint32_t pid,
    uint32_t irp_va){
    return queue_ast(context,pid,0u,irp_va);
}

/* IOCIOPOST's direct-I/O route already resolved the PCB. Recheck that it is
 * still the PID's vector entry before committing the KAST. */
uint32_t port_v37_iopost_sched_queue_kast(void *context,uint32_t pid,
    uint32_t pcb_va,uint32_t irp_va){
    return queue_ast(context,pid,pcb_va,irp_va);
}
