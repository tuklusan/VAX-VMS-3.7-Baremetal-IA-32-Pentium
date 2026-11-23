/* Native IA-32 partial translation: FORIDATE.MAR, pinned VMS 3.7.
 * Original lines 1..136; original entries: FOR$IDATE.
 * time source semantics are represented in explicit native helpers.
 * Copyrighted DEC source text is NOT copied into this file.
 * VAX calling frame, exact condition signals and live guest binding pending.
 */
#include "../vms_rtl_m111.h"
int port111_foridate(const struct port111_clock *t,int16_t *m,int16_t *d,int16_t *y){return port111_mdy(t,m,d,y,16);}
