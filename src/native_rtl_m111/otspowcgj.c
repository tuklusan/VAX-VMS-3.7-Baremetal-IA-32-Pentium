/* Native IA-32 partial translation: OTSPOWCGJ.MAR, pinned VMS 3.7.
 * Original lines 1..220; original entries: OTS$POWCGJ_R3.
 * complex_intpow source semantics are represented in explicit native helpers.
 * Copyrighted DEC source text is NOT copied into this file.
 * VAX calling frame, exact condition signals and live guest binding pending.
 */
#include "../vms_rtl_m111.h"
int port111_otspowcgj(const uint16_t *a,int32_t exp,uint16_t *out){return port111_complex_intpow(PORT_M107_G,a,exp,out);}
