/* Native IA-32 partial translation: OTSPOWHH.MAR, pinned VMS 3.7.
 * Original lines 1..185; original entries: OTS$POWHH_R3.
 * real_pow source semantics are represented in explicit native helpers.
 * Copyrighted DEC source text is NOT copied into this file.
 * VAX calling frame, exact condition signals and live guest binding pending.
 */
#include "../vms_rtl_m111.h"
int port111_otspowhh(const uint16_t *a,const uint16_t *b,uint16_t *out){return port111_real_pow(PORT_M107_H,a,b,out,0);}
