/* Native IA-32 partial translation: BASMULDDI.MAR, pinned VMS 3.7.
 * Original lines 1..202; original entries: BAS$$MULD, BAS$$DIVD.
 * basic_arith source semantics are represented in explicit native helpers.
 * Copyrighted DEC source text is NOT copied into this file.
 * VAX calling frame, exact condition signals and live guest binding pending.
 */
#include "../vms_rtl_m111.h"
int port111_basmulddi_mul(const uint16_t *a,const uint16_t *b,uint16_t *out){return port111_d_arith(a,b,out,0);}
int port111_basmulddi_div(const uint16_t *a,const uint16_t *b,uint16_t *out){return port111_d_arith(a,b,out,1);}
