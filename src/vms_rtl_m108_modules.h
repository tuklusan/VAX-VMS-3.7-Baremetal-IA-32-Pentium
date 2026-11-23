#ifndef VMS_RTL_M108_MODULES_H
#define VMS_RTL_M108_MODULES_H
#include "vms_rtl_m108.h"
int port108_forrandom(uint16_t *high,uint16_t *low,uint16_t out[2]);
uint32_t port108_libcrc(uint32_t polynomial,uint32_t initial,const uint8_t *data,size_t bytes);
int port108_libediv(int32_t divisor,int64_t dividend,int32_t *quot,int32_t *rem);
int port108_libemul(int32_t multiplier,int32_t multiplicand,int32_t addend,int64_t *result);
int port108_libgetopc(const uint8_t *instruction,size_t readable,uint16_t *opcode);
int port108_libinsqhi(struct port108_queue *q,uint32_t entry);
int port108_libinsqti(struct port108_queue *q,uint32_t entry);
int port108_libmovtc(const uint8_t *src,size_t ns,const uint8_t table[256],uint8_t fill,uint8_t *dest,size_t nd);
int port108_libmovtuc(const uint8_t *src,size_t ns,const uint8_t table[256],uint8_t escape,uint8_t fill,uint8_t *dest,size_t nd,size_t *index);
int port108_libremqhi(struct port108_queue *q,uint32_t *removed);
int port108_libremqti(struct port108_queue *q,uint32_t *removed);
int port108_mthabs_i2(int16_t x,int16_t *out);
int port108_mthabs_i4(int32_t x,int32_t *out);
int port108_mthabs_f(enum port_m107_fmt f,const uint16_t *x,uint16_t *out);
int port108_mthaint(const uint16_t *v,uint16_t *out);
int port108_mthamod(const uint16_t *a,const uint16_t *b,uint16_t *out);
int port108_mthanint(const uint16_t *v,uint16_t *out);
int port108_mthbitops_i2(unsigned operation,uint16_t a,uint16_t b,uint16_t *out);
int port108_mthbitops_i4(unsigned operation,uint32_t a,uint32_t b,uint32_t *out);
int port108_mthcabs(const uint16_t *pair,uint16_t *out);
int port108_mthcdabs(const uint16_t *pair,uint16_t *out);
int port108_mthcgabs(const uint16_t *pair,uint16_t *out);
int port108_mthdfloor(const uint16_t *v,uint16_t *out);
int port108_mthdim_i2(int16_t a,int16_t b,int16_t *out);
int port108_mthdim_i4(int32_t a,int32_t b,int32_t *out);
int port108_mthdim_float(enum port_m107_fmt f,const uint16_t *a,const uint16_t *b,uint16_t *out);
int port108_mthdint(const uint16_t *v,uint16_t *out);
int port108_mthdmod(const uint16_t *a,const uint16_t *b,uint16_t *out);
int port108_mthdnint(const uint16_t *v,uint16_t *out);
int port108_mthdprod(const uint16_t a[2],const uint16_t b[2],uint16_t *out);
int port108_mthfloor(const uint16_t *v,uint16_t *out);
int port108_mthgfloor(const uint16_t *v,uint16_t *out);
int port108_mthgint(const uint16_t *v,uint16_t *out);
int port108_mthgmod(const uint16_t *a,const uint16_t *b,uint16_t *out);
int port108_mthgnint(const uint16_t *v,uint16_t *out);
int port108_mthgprod(const uint16_t a[2],const uint16_t b[2],uint16_t *out);
int port108_mthhfloor(const uint16_t *v,uint16_t *out);
int port108_mthhint(const uint16_t *v,uint16_t *out);
int port108_mthhmod(const uint16_t *a,const uint16_t *b,uint16_t *out);
int port108_mthhnint(const uint16_t *v,uint16_t *out);
int port108_mthmod_i2(int16_t a,int16_t b,int16_t *out);
int port108_mthmod_i4(int32_t a,int32_t b,int32_t *out);
int port108_mthrandom(uint32_t *seed,uint16_t out[2]);
int port108_strmovq(const uint8_t *src,uint8_t *dst);
#endif
