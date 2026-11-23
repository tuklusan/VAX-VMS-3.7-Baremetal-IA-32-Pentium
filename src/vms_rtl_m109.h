#ifndef VMS_RTL_M109_H
#define VMS_RTL_M109_H
#include <stdint.h>
#include <stddef.h>
#include "vms_rtl_m107.h"
/* Native IA-32 VAX F/D/G/H transcendentals; x87 computes
 * useful results, but VAX polynomial tables and exception frames
 * are still NOT reproduced. APPROX is successful, not fidelity proof. */
enum {PORT109_OK=1,PORT109_BAD_ARG=2,PORT109_APPROX=3,PORT109_PARTIAL=4,
      PORT109_RANGE=6,PORT109_RESERVED=8,PORT109_DOMAIN=10};
enum port109_function {PORT109_ACOS,PORT109_ASIN,PORT109_ATAN,
 PORT109_LOG,PORT109_EXP,PORT109_SQRT,PORT109_SINH,PORT109_COSH,
 PORT109_TANH,PORT109_LOG10,PORT109_LOG2};
int port109_decode(enum port_m107_fmt fmt,const uint16_t *raw,long double *out);
int port109_encode(enum port_m107_fmt fmt,long double value,uint16_t *out);
int port109_unary(enum port_m107_fmt fmt,enum port109_function op,const uint16_t *arg,uint16_t *out);
int port109_unary_degrees(enum port_m107_fmt fmt,enum port109_function op,const uint16_t *arg,uint16_t *out);
int port109_atan_ratio(enum port_m107_fmt fmt,const uint16_t *numerator,
                       const uint16_t *denominator,int degrees,uint16_t *out);
/* One native .c per original DEC source file, distinct callable symbols. */
int port109_mthacos(const uint16_t *arg,uint16_t *out);
int port109_mthacosd(const uint16_t *arg,uint16_t *out);
int port109_mthdacos(const uint16_t *arg,uint16_t *out);
int port109_mthdacosd(const uint16_t *arg,uint16_t *out);
int port109_mthgacos(const uint16_t *arg,uint16_t *out);
int port109_mthgacosd(const uint16_t *arg,uint16_t *out);
int port109_mthhacos(const uint16_t *arg,uint16_t *out);
int port109_mthhacosd(const uint16_t *arg,uint16_t *out);
int port109_mthasin(const uint16_t *arg,uint16_t *out);
int port109_mthasind(const uint16_t *arg,uint16_t *out);
int port109_mthdasin(const uint16_t *arg,uint16_t *out);
int port109_mthdasind(const uint16_t *arg,uint16_t *out);
int port109_mthgasin(const uint16_t *arg,uint16_t *out);
int port109_mthgasind(const uint16_t *arg,uint16_t *out);
int port109_mthhasin(const uint16_t *arg,uint16_t *out);
int port109_mthhasind(const uint16_t *arg,uint16_t *out);
int port109_mthatan(const uint16_t *arg,uint16_t *out);
int port109_mthatand(const uint16_t *arg,uint16_t *out);
int port109_mthatan2(const uint16_t *num,const uint16_t *den,uint16_t *out);
int port109_mthatand2(const uint16_t *num,const uint16_t *den,uint16_t *out);
int port109_mthdatan(const uint16_t *arg,uint16_t *out);
int port109_mthdatand(const uint16_t *arg,uint16_t *out);
int port109_mthdatan2(const uint16_t *num,const uint16_t *den,uint16_t *out);
int port109_mthdatand2(const uint16_t *num,const uint16_t *den,uint16_t *out);
int port109_mthgatan(const uint16_t *arg,uint16_t *out);
int port109_mthgatand(const uint16_t *arg,uint16_t *out);
int port109_mthgatan2(const uint16_t *num,const uint16_t *den,uint16_t *out);
int port109_mthgatand2(const uint16_t *num,const uint16_t *den,uint16_t *out);
int port109_mthhatan(const uint16_t *arg,uint16_t *out);
int port109_mthhatand(const uint16_t *arg,uint16_t *out);
int port109_mthhatan2(const uint16_t *num,const uint16_t *den,uint16_t *out);
int port109_mthhatand2(const uint16_t *num,const uint16_t *den,uint16_t *out);
int port109_mthalog(const uint16_t *arg,uint16_t *out);
int port109_mthalog10(const uint16_t *arg,uint16_t *out);
int port109_mthalog2(const uint16_t *arg,uint16_t *out);
int port109_mthdlog(const uint16_t *arg,uint16_t *out);
int port109_mthdlog10(const uint16_t *arg,uint16_t *out);
int port109_mthdlog2(const uint16_t *arg,uint16_t *out);
int port109_mthglog(const uint16_t *arg,uint16_t *out);
int port109_mthglog10(const uint16_t *arg,uint16_t *out);
int port109_mthglog2(const uint16_t *arg,uint16_t *out);
int port109_mthhlog(const uint16_t *arg,uint16_t *out);
int port109_mthhlog10(const uint16_t *arg,uint16_t *out);
int port109_mthhlog2(const uint16_t *arg,uint16_t *out);
int port109_mthexp(const uint16_t *arg,uint16_t *out);
int port109_mthdexp(const uint16_t *arg,uint16_t *out);
int port109_mthgexp(const uint16_t *arg,uint16_t *out);
int port109_mthhexp(const uint16_t *arg,uint16_t *out);
int port109_mthsqrt(const uint16_t *arg,uint16_t *out);
int port109_mthdsqrt(const uint16_t *arg,uint16_t *out);
int port109_mthgsqrt(const uint16_t *arg,uint16_t *out);
int port109_mthhsqrt(const uint16_t *arg,uint16_t *out);
int port109_mthsinh(const uint16_t *arg,uint16_t *out);
int port109_mthdsinh(const uint16_t *arg,uint16_t *out);
int port109_mthgsinh(const uint16_t *arg,uint16_t *out);
int port109_mthhsinh(const uint16_t *arg,uint16_t *out);
int port109_mthcosh(const uint16_t *arg,uint16_t *out);
int port109_mthdcosh(const uint16_t *arg,uint16_t *out);
int port109_mthgcosh(const uint16_t *arg,uint16_t *out);
int port109_mthhcosh(const uint16_t *arg,uint16_t *out);
int port109_mthtanh(const uint16_t *arg,uint16_t *out);
int port109_mthdtanh(const uint16_t *arg,uint16_t *out);
int port109_mthgtanh(const uint16_t *arg,uint16_t *out);
int port109_mthhtanh(const uint16_t *arg,uint16_t *out);
#endif
