/* Native IA-32 partial translation: BASPOWDD.MAR, pinned VMS 3.7.
 * Original lines 1..202; original entries: BAS$POWDD.
 * basic_pow source semantics are represented in explicit native helpers.
 * Copyrighted DEC source text is NOT copied into this file.
 * VAX calling frame, exact condition signals and live guest binding pending.
 */
#include "../vms_rtl_m111.h"
int port111_baspowdd(const uint16_t *a,const uint16_t *e,uint16_t *out){return port111_real_pow(PORT_M107_D,a,e,out,1);}
