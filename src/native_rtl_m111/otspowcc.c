/* Native IA-32 partial translation: OTSPOWCC.MAR, pinned VMS 3.7.
 * Original lines 1..176; original entries: OTS$POWCC.
 * complex_pow source semantics are represented in explicit native helpers.
 * Copyrighted DEC source text is NOT copied into this file.
 * VAX calling frame, exact condition signals and live guest binding pending.
 */
#include "../vms_rtl_m111.h"
int port111_otspowcc(const uint16_t *a,const uint16_t *b,uint16_t *out){return port111_complex_pow(PORT_M107_F,a,b,out);}
