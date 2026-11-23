#include "vms_dq_fdt.h"
/* Original DRIVER/SRC/DQDRIVER.MAR 554-561 FUNCTAB source class: WRITE.
 * Each bit position comes from VMSLIB/SRC/STARDEFFL.MDL (IO$_VIRTUAL=63).
 * No action address or driver is invented at this stage. */
uint64_t port_dq_fdt_write(void){
    return PORT_DQ_BIT(10) |
        PORT_DQ_BIT(13) |
        PORT_DQ_BIT(32) |
        PORT_DQ_BIT(11) |
        PORT_DQ_BIT(48);
}
