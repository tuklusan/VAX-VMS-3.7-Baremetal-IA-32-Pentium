#ifndef VMS37_RTL_M111_H
#define VMS37_RTL_M111_H
#include <stddef.h>
#include <stdint.h>
#include "vms_rtl_m109.h"
/* Source-specific IA-32 RTL routines. Inputs are VAX F/D/G/H 16-bit-word
 * encodings, never an emulated VAX register file. The live guest ABI,
 * signal dispatch, descriptor traps and integration remain UNIMPLEMENTED.
 * Partial/approximation return statuses must not be interpreted as VMS SS$_NORMAL.
 */
struct port111_clock {unsigned year,month,day,hour,minute,second,centisecond;};
struct port111_lun {uint32_t bits[4];};
struct port111_det {uint16_t d[4];};
int port111_clock_valid(const struct port111_clock *clock);
int port111_date(const struct port111_clock *clock,char *out,size_t capacity,int is_time);
int port111_date_descriptor(const struct port111_clock *clock,char *out,size_t cap,int is_time);
int port111_mdy(const struct port111_clock *clock,void *month,void *day,void *year,unsigned bits);
int port111_seconds(const struct port111_clock *clock,const uint16_t *start,uint16_t *out);
int port111_complex_mul(enum port_m107_fmt fmt,const uint16_t *left,const uint16_t *right,uint16_t *result);
int port111_complex_div(enum port_m107_fmt fmt,const uint16_t *left,const uint16_t *right,uint16_t *result);
int port111_complex_pow(enum port_m107_fmt fmt,const uint16_t *base,const uint16_t *exponent,uint16_t *result);
int port111_complex_intpow(enum port_m107_fmt fmt,const uint16_t *base,int32_t exponent,uint16_t *result);
int port111_real_pow(enum port_m107_fmt fmt,const uint16_t *base,const uint16_t *exponent,uint16_t *result,int basic_semantics);
int port111_real_intpow(enum port_m107_fmt fmt,const uint16_t *base,int32_t exponent,uint16_t *result,int basic_semantics);
int port111_integer_pow(int64_t base,int64_t exponent,unsigned bits,int64_t *result);
int port111_ulogpow(uint32_t base,uint32_t exponent,uint32_t *out);
void port111_lun_init(struct port111_lun *pool);
int port111_lun_get(struct port111_lun *pool,unsigned *unit);
int port111_lun_free(struct port111_lun *pool,unsigned unit);
int port111_lun_take(struct port111_lun *pool,unsigned unit);
int port111_parse_logical(const char *str,size_t len,unsigned bits,void *out);
int port111_parse_integer(const char *str,size_t len,unsigned bits,int allow_tabs,void *out);
int port111_move(const void *src,void *dst,size_t n);
int port111_d_arith(const uint16_t *a,const uint16_t *b,uint16_t *out,int divide);
int port111_copy_float(enum port_m107_fmt fmt,const uint16_t *source,uint16_t *target);
int port111_det_store(struct port111_det *d,const uint16_t *value);
int port111_det_get(const struct port111_det *d,enum port_m107_fmt fmt,uint16_t *out);
int port111_xlate(const uint8_t *src,size_t n,const uint8_t *table,size_t table_n,
                  uint8_t *dst,size_t capacity,size_t *written);
int port111_mulq(const uint32_t a[2],const uint32_t b[2],uint32_t result[2]);
/* Every source module has a separate native symbol and .c file. */
int port111_fortime(const struct port111_clock *t,char out[8]);
int port111_fortimeds(const struct port111_clock *t,char *out,size_t cap);
int port111_forjdate(const struct port111_clock *t,int32_t *m,int32_t *d,int32_t *y);
int port111_foridate(const struct port111_clock *t,int16_t *m,int16_t *d,int16_t *y);
int port111_fordate(const struct port111_clock *t,char out[9]);
int port111_fordateds(const struct port111_clock *t,char *out,size_t cap);
int port111_forsecnds(const struct port111_clock *t,const uint16_t *start,uint16_t *out);
int port111_otsmulcd(const uint16_t *a,const uint16_t *b,uint16_t *out);
int port111_otsmulcg(const uint16_t *a,const uint16_t *b,uint16_t *out);
int port111_otsdivc(const uint16_t *a,const uint16_t *b,uint16_t *out);
int port111_otsdivcd(const uint16_t *a,const uint16_t *b,uint16_t *out);
int port111_otsdivcg(const uint16_t *a,const uint16_t *b,uint16_t *out);
int port111_otspowcc(const uint16_t *a,const uint16_t *b,uint16_t *out);
int port111_otspowcdc(const uint16_t *a,const uint16_t *b,uint16_t *out);
int port111_otspowcgc(const uint16_t *a,const uint16_t *b,uint16_t *out);
int port111_otspowlul(uint32_t a,uint32_t b,uint32_t *out);
int port111_otspowhh(const uint16_t *a,const uint16_t *b,uint16_t *out);
int port111_otspowgg(const uint16_t *a,const uint16_t *b,uint16_t *out);
int port111_otspowrr(const uint16_t *a,const uint16_t *b,uint16_t *out);
int port111_otspowcdj(const uint16_t *a,int32_t exp,uint16_t *out);
int port111_otspowcgj(const uint16_t *a,int32_t exp,uint16_t *out);
int port111_otspowii(int16_t a,int16_t b,int16_t *out);
int port111_otspowdr(const uint16_t *base_d,const uint16_t *exp_f,uint16_t *out);
int port111_otspowrd(const uint16_t *base_f,const uint16_t *exp_d,uint16_t *out);
int port111_otspowdd(const uint16_t *base_d,const uint16_t *exp_d,uint16_t *out);
int port111_otspowjj(int32_t a,int32_t b,int32_t *out);
int port111_otspowcj(const uint16_t *a,int32_t exp,uint16_t *out);
int port111_otslun_get(struct port111_lun *p,unsigned *unit);
int port111_otslun_free(struct port111_lun *p,unsigned unit);
int port111_otslun_take(struct port111_lun *p,unsigned unit);
int port111_otscvttll(const char *src,size_t n,unsigned bits,void *out);
int port111_otscvttll_compat(const char *src,size_t n,unsigned bits,void *out);
int port111_otscvttil(const char *src,size_t n,unsigned bits,int tabs,void *out);
int port111_otscvttil_compat(const char *src,size_t n,unsigned bits,int tabs,void *out);
int port111_otsmove(const void *src,void *dst,size_t n);
int port111_otsmove_r5(const void *src,void *dst,size_t n);
int port111_baspowrj(const uint16_t *base,int32_t exp,uint16_t *out);
int port111_baspowdj(const uint16_t *base,int32_t exp,uint16_t *out);
int port111_baspowjj(int32_t base,int32_t exp,int32_t *out);
int port111_baspowii(int16_t base,int16_t exp,int16_t *out);
int port111_baspowrr(const uint16_t *base,const uint16_t *exp,uint16_t *out);
int port111_baspowdd(const uint16_t *base,const uint16_t *exp,uint16_t *out);
int port111_basmulddi_mul(const uint16_t *a,const uint16_t *b,uint16_t *out);
int port111_basmulddi_div(const uint16_t *a,const uint16_t *b,uint16_t *out);
int port111_bascopyfd_f(const uint16_t *src,uint16_t *out);
int port111_bascopyfd_d(const uint16_t *src,uint16_t *out);
int port111_basdet_store(struct port111_det *s,const uint16_t *v);
int port111_basdet_d(const struct port111_det *s,uint16_t *out);
int port111_basdet_f(const struct port111_det *s,uint16_t *out);
int port111_basxlate(const uint8_t *src,size_t n,const uint8_t *table,size_t table_n,
                     uint8_t *dst,size_t cap,size_t *written);
int port111_cobmulq(const uint32_t a[2],const uint32_t b[2],uint32_t out[2]);
#endif
