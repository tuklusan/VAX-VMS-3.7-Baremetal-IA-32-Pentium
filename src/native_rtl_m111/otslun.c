/* Native IA-32 partial translation: OTSLUN.MAR, pinned VMS 3.7.
 * Original lines 1..337; original entries: OTS$$GET_LUN, OTS$$FREE_LUN, OTS$$TAKE_LUN.
 * lun source semantics are represented in explicit native helpers.
 * Copyrighted DEC source text is NOT copied into this file.
 * VAX calling frame, exact condition signals and live guest binding pending.
 */
#include "../vms_rtl_m111.h"
int port111_otslun_get(struct port111_lun *pool,unsigned *unit){return port111_lun_get(pool,unit);}
int port111_otslun_free(struct port111_lun *pool,unsigned unit){return port111_lun_free(pool,unit);}
int port111_otslun_take(struct port111_lun *pool,unsigned unit){return port111_lun_take(pool,unit);}
