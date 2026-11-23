#include "vms_dq_fdt.h"
/* Original DRIVER/SRC/DQDRIVER.MAR 587-590 FUNCTAB source class: SENSE.
 * Each bit position comes from VMSLIB/SRC/STARDEFFL.MDL (IO$_VIRTUAL=63).
 * No action address or driver is invented at this stage. */
uint64_t port_dq_fdt_sense(void){
    return PORT_DQ_BIT(27) |
        PORT_DQ_BIT(39);
}
