/* Native IA-32 partial translation: FORSECNDS.MAR, pinned VMS 3.7.
 * Original lines 1..150; original entries: FOR$SECNDS.
 * time source semantics are represented in explicit native helpers.
 * Copyrighted DEC source text is NOT copied into this file.
 * VAX calling frame, exact condition signals and live guest binding pending.
 */
#include "../vms_rtl_m111.h"
int port111_forsecnds(const struct port111_clock *t,const uint16_t *start,uint16_t *out){return port111_seconds(t,start,out);}
