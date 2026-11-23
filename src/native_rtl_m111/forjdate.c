/* Native IA-32 partial translation: FORJDATE.MAR, pinned VMS 3.7.
 * Original lines 1..135; original entries: FOR$JDATE.
 * time source semantics are represented in explicit native helpers.
 * Copyrighted DEC source text is NOT copied into this file.
 * VAX calling frame, exact condition signals and live guest binding pending.
 */
#include "../vms_rtl_m111.h"
int port111_forjdate(const struct port111_clock *t,int32_t *m,int32_t *d,int32_t *y){return port111_mdy(t,m,d,y,32);}
