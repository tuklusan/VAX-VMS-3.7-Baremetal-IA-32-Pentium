/* Native IA-32 partial translation: BASPOWRJ.MAR, pinned VMS 3.7.
 * Original lines 1..183; original entries: BAS$POWRJ.
 * basic_pow source semantics are represented in explicit native helpers.
 * Copyrighted DEC source text is NOT copied into this file.
 * VAX calling frame, exact condition signals and live guest binding pending.
 */
#include "../vms_rtl_m111.h"
int port111_baspowrj(const uint16_t *a,int32_t exp,uint16_t *out){return port111_real_intpow(PORT_M107_F,a,exp,out,1);}
