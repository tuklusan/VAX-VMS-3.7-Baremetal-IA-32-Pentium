/* Native IA-32 partial translation: OTSPOWJJ.MAR, pinned VMS 3.7.
 * Original lines 1..243; original entries: OTS$POWJJ.
 * integer_pow source semantics are represented in explicit native helpers.
 * Copyrighted DEC source text is NOT copied into this file.
 * VAX calling frame, exact condition signals and live guest binding pending.
 */
#include "../vms_rtl_m111.h"
int port111_otspowjj(int32_t a,int32_t b,int32_t *out){int64_t v=0;int st=port111_integer_pow(a,b,32,&v);if(st==PORT109_OK&&out)*out=(int32_t)v;return out?st:PORT109_BAD_ARG;}
