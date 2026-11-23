/* Native IA-32 partial translation: OTSMOVE.MAR, pinned VMS 3.7.
 * Original lines 1..219; original entries: OTS$MOVE3, OTS$MOVE3_R5.
 * memory_move source semantics are represented in explicit native helpers.
 * Copyrighted DEC source text is NOT copied into this file.
 * VAX calling frame, exact condition signals and live guest binding pending.
 */
#include "../vms_rtl_m111.h"
int port111_otsmove(const void *src,void *dst,size_t n){return port111_move(src,dst,n);}
int port111_otsmove_r5(const void *src,void *dst,size_t n){return port111_move(src,dst,n);}
