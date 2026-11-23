/* Native IA-32 partial translation: FORDATEDS.MAR, pinned VMS 3.7.
 * Original lines 1..138; original entries: FOR$DATE_T_DS.
 * time source semantics are represented in explicit native helpers.
 * Copyrighted DEC source text is NOT copied into this file.
 * VAX calling frame, exact condition signals and live guest binding pending.
 */
#include "../vms_rtl_m111.h"
int port111_fordateds(const struct port111_clock *t,char *out,size_t cap){return port111_date_descriptor(t,out,cap,0);}
