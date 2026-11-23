#include "vms_ini_dpt_templates.h"
/* SYS/SRC/MBDRIVER.MAR 107-119. Source prologue only; no hardware driver or IOC posting. */
uint32_t port_v37_dpt_stage_mb(uint8_t record[PORT_V37_DPT_STAGE_BYTES]){
    return port_v37_dpt_fill(record,PORT_V37_DPT_STAGE_BYTES,PORT_V37_DPT_MB_UCBSIZE,"MBDRIVER",8u);
}
