#include "vms_ini_devicedb.h"
/* DEVICEDAT.MAR 353-355: mailbox DDB with the real prelinked UCB1 head.
 * Source MB$DDT is not linked; keep DDB$L_DDT zero and fail closed. */
void port_v37_ddb_mb(uint8_t *d,uint32_t next,uint32_t ucb){
    port_v37_ddb_base(d,next,ucb,"MBA",3u,"MBDRIVER",8u);
}
