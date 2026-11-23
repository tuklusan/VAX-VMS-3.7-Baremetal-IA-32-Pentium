/* Native IA-32 source-specific partial entry; original: RTL/SRC/BASMATRED.MAR, line 153.
 * No DEC source is bundled. VAX calling mask, raised conditions, RTL integration not yet reproduced. */
#include "../vms_rtl_m113.h"
int port113_src_basmatred(struct port113_matrix *m,uint32_t rows,uint32_t cols){return port113_matrix_redim(m,rows,cols);}
