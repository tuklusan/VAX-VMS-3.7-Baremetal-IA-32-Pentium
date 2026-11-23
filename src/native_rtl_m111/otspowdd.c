/* Native IA-32 partial translation: OTSPOWDD.MAR, pinned VMS 3.7.
 * Original lines 1..226; original entries: OTS$POWDR, OTS$POWRD, OTS$POWDD.
 * real_pow source semantics are represented in explicit native helpers.
 * Copyrighted DEC source text is NOT copied into this file.
 * VAX calling frame, exact condition signals and live guest binding pending.
 */
#include "../vms_rtl_m111.h"
int port111_otspowdr(const uint16_t *base_d,const uint16_t *exp_f,uint16_t *out){
    if(!base_d||!exp_f||!out)return PORT109_BAD_ARG;
    long double e;int st=port109_decode(PORT_M107_F,exp_f,&e);if(st!=PORT109_OK)return st;
    uint16_t temp[4]={0};st=port109_encode(PORT_M107_D,e,temp);if(st!=PORT109_OK)return st;
    return port111_real_pow(PORT_M107_D,base_d,temp,out,0);
}
int port111_otspowrd(const uint16_t *base_f,const uint16_t *exp_d,uint16_t *out){
    if(!base_f||!exp_d||!out)return PORT109_BAD_ARG;
    long double b;int st=port109_decode(PORT_M107_F,base_f,&b);if(st!=PORT109_OK)return st;
    uint16_t temp[4]={0};st=port109_encode(PORT_M107_D,b,temp);if(st!=PORT109_OK)return st;
    return port111_real_pow(PORT_M107_D,temp,exp_d,out,0);
}
int port111_otspowdd(const uint16_t *base_d,const uint16_t *exp_d,uint16_t *out){return port111_real_pow(PORT_M107_D,base_d,exp_d,out,0);}
