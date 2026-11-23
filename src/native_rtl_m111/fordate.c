/* Native IA-32 partial translation: FORDATE.MAR, pinned VMS 3.7.
 * Original lines 1..137; original entries: FOR$DATE.
 * time source semantics are represented in explicit native helpers.
 * Copyrighted DEC source text is NOT copied into this file.
 * VAX calling frame, exact condition signals and live guest binding pending.
 */
#include "../vms_rtl_m111.h"
int port111_fordate(const struct port111_clock *t,char out[9]){return port111_date(t,out,9,0);}
