/* Native IA-32 partial translation: BASDET.MAR, pinned VMS 3.7.
 * Original lines 1..227; original entries: BAS$DET_F, BAS$DET_D, BAS$$STORE_DET.
 * basic_det source semantics are represented in explicit native helpers.
 * Copyrighted DEC source text is NOT copied into this file.
 * VAX calling frame, exact condition signals and live guest binding pending.
 */
#include "../vms_rtl_m111.h"
int port111_basdet_store(struct port111_det *d,const uint16_t *value){return port111_det_store(d,value);}
int port111_basdet_d(const struct port111_det *d,uint16_t *out){return port111_det_get(d,PORT_M107_D,out);}
int port111_basdet_f(const struct port111_det *d,uint16_t *out){return port111_det_get(d,PORT_M107_F,out);}
