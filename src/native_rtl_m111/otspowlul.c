/* Native IA-32 partial translation: OTSPOWLUL.MAR, pinned VMS 3.7.
 * Original lines 1..149; original entries: OTS$POWLULU.
 * integer_pow source semantics are represented in explicit native helpers.
 * Copyrighted DEC source text is NOT copied into this file.
 * VAX calling frame, exact condition signals and live guest binding pending.
 */
#include "../vms_rtl_m111.h"
int port111_otspowlul(uint32_t a,uint32_t b,uint32_t *out){return port111_ulogpow(a,b,out);}
