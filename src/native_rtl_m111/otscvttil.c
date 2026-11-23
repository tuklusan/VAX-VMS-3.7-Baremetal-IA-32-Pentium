/* Native IA-32 partial translation: OTSCVTTIL.MAR, pinned VMS 3.7.
 * Original lines 1..301; original entries: FOR$CNV_IN_I, OTS$CVT_TI_L.
 * text_integer source semantics are represented in explicit native helpers.
 * Copyrighted DEC source text is NOT copied into this file.
 * VAX calling frame, exact condition signals and live guest binding pending.
 */
#include "../vms_rtl_m111.h"
int port111_otscvttil(const char *src,size_t n,unsigned bits,int tabs,void *out){return port111_parse_integer(src,n,bits,tabs,out);}
int port111_otscvttil_compat(const char *src,size_t n,unsigned bits,int tabs,void *out){return port111_otscvttil(src,n,bits,tabs,out);}
