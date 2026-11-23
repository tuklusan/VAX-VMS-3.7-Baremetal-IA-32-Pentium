#include "vms_dqddt.h"
/* FUNCTAB emits two 32-bit mask words per row; the ten nonempty ACTION
 * entries additionally reserve one GENRADDR longword. */
uint32_t port_dqddt_entry_offset(unsigned row){
    if(row>=PORT_DQ_FDT_ROWS)return UINT32_MAX;
    if(row<2u)return PORT_DQDDT_HEADER_BYTES+8u*row;
    return PORT_DQDDT_HEADER_BYTES+16u+12u*(row-2u);
}
