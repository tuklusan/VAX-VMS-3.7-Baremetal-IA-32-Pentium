/* Native IA-32 partial translation: OTSCVTTLL.MAR, pinned VMS 3.7.
 * Original lines 1..215; original entries: FOR$CNV_IN_L, OTS$CVT_TL_L.
 * logical source semantics are represented in explicit native helpers.
 * Copyrighted DEC source text is NOT copied into this file.
 * VAX calling frame, exact condition signals and live guest binding pending.
 */
#include "../vms_rtl_m111.h"
int port111_otscvttll(const char *src,size_t n,unsigned bits,void *out){return port111_parse_logical(src,n,bits,out);}
int port111_otscvttll_compat(const char *src,size_t n,unsigned bits,void *out){return port111_otscvttll(src,n,bits,out);}
