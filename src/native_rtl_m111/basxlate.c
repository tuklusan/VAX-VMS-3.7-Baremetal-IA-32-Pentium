/* Native IA-32 partial translation: BASXLATE.MAR, pinned VMS 3.7.
 * Original lines 1..242; original entries: BAS$XLATE.
 * basic_translate source semantics are represented in explicit native helpers.
 * Copyrighted DEC source text is NOT copied into this file.
 * VAX calling frame, exact condition signals and live guest binding pending.
 */
#include "../vms_rtl_m111.h"
int port111_basxlate(const uint8_t *src,size_t n,const uint8_t *table,size_t table_n,uint8_t *dst,size_t cap,size_t *written){return port111_xlate(src,n,table,table_n,dst,cap,written);}
