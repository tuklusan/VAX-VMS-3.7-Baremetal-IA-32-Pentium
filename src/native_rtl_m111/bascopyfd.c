/* Native IA-32 partial translation: BASCOPYFD.MAR, pinned VMS 3.7.
 * Original lines 1..179; original entries: BAS$$COPY_F_R1, BAS$$COPY_D_R1.
 * basic_copy source semantics are represented in explicit native helpers.
 * Copyrighted DEC source text is NOT copied into this file.
 * VAX calling frame, exact condition signals and live guest binding pending.
 */
#include "../vms_rtl_m111.h"
int port111_bascopyfd_f(const uint16_t *src,uint16_t *out){return port111_copy_float(PORT_M107_F,src,out);}
int port111_bascopyfd_d(const uint16_t *src,uint16_t *out){return port111_copy_float(PORT_M107_D,src,out);}
