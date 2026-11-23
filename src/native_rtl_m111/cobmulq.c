/* Native IA-32 partial translation: COBMULQ.MAR, pinned VMS 3.7.
 * Original lines 1..151; original entries: COB$MULQ_R8.
 * quad_mul source semantics are represented in explicit native helpers.
 * Copyrighted DEC source text is NOT copied into this file.
 * VAX calling frame, exact condition signals and live guest binding pending.
 */
#include "../vms_rtl_m111.h"
int port111_cobmulq(const uint32_t a[2],const uint32_t b[2],uint32_t out[2]){return port111_mulq(a,b,out);}
