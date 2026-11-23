/* Native IA-32 partial translation: BASPOWII.MAR, pinned VMS 3.7.
 * Original lines 1..185; original entries: BAS$POWII.
 * basic_pow source semantics are represented in explicit native helpers.
 * Copyrighted DEC source text is NOT copied into this file.
 * VAX calling frame, exact condition signals and live guest binding pending.
 */
#include "../vms_rtl_m111.h"
int port111_baspowii(int16_t a,int16_t b,int16_t *out){int64_t v=0;int st=port111_integer_pow(a,b,16,&v);if(st==PORT109_OK&&out)*out=(int16_t)v;return out?st:PORT109_BAD_ARG;}
