#include "vms_dq_fdt.h"
/* Original DRIVER/SRC/DQDRIVER.MAR 528-545 FUNCTAB source class: BUFFERED.
 * Each bit position comes from VMSLIB/SRC/STARDEFFL.MDL (IO$_VIRTUAL=63).
 * No action address or driver is invented at this stage. */
uint64_t port_dq_fdt_buffered(void){
    return PORT_DQ_BIT(0) |
        PORT_DQ_BIT(1) |
        PORT_DQ_BIT(2) |
        PORT_DQ_BIT(3) |
        PORT_DQ_BIT(4) |
        PORT_DQ_BIT(8) |
        PORT_DQ_BIT(17) |
        PORT_DQ_BIT(26) |
        PORT_DQ_BIT(27) |
        PORT_DQ_BIT(35) |
        PORT_DQ_BIT(39) |
        PORT_DQ_BIT(50) |
        PORT_DQ_BIT(51) |
        PORT_DQ_BIT(52) |
        PORT_DQ_BIT(53) |
        PORT_DQ_BIT(54) |
        PORT_DQ_BIT(56) |
        PORT_DQ_BIT(57);
}
