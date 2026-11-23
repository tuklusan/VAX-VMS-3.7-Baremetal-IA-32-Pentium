#ifndef PORTLAB_VMS_RTL_M110_H
#define PORTLAB_VMS_RTL_M110_H
#include <stdint.h>
#include "vms_rtl_m109.h"
/* Source-derived native numerical services. These are host-only ELF32
 * relocatable RTL objects, with VAX register/condition semantics outstanding.
 * Exactly one source wrapper per pinned DEC module, no DEC text reproduced.
 */
enum port110_complex_op {PORT110_CEXP,PORT110_CLOG,PORT110_CSQRT,
                         PORT110_CSIN,PORT110_CCOS};
int port110_complex(enum port_m107_fmt fmt,enum port110_complex_op op,
                    const uint16_t *a,uint16_t *out);
int port110_atanh(enum port_m107_fmt fmt,const uint16_t *a,uint16_t *out);
int port110_sincos(enum port_m107_fmt fmt,int degrees,const uint16_t *a,
                   uint16_t *s,uint16_t *c);
int port110_poly(enum port_m107_fmt fmt,const uint16_t *x,
                 unsigned order,const uint16_t *coeff,uint16_t *out);
int port110_emod(enum port_m107_fmt fmt,const uint16_t *mulr,
                 uint16_t extension,const uint16_t *muld,int32_t *integral,
                 uint16_t *fraction);
int port110_convert(enum port_m107_fmt src,enum port_m107_fmt dst,
                    const uint16_t *a,uint16_t *out);
int port110_float_to_int(enum port_m107_fmt fmt,const uint16_t *a,
                         unsigned bits,int32_t *result);
int port110_int_to_float(enum port_m107_fmt fmt,int32_t value,uint16_t *out);
/* The original text-table services have special escape/signal paths.
 * This strict alphanumeric/space subset rejects every other byte rather
 * than silently claiming DEC's complete translation-table equivalence. */
int port110_asc_to_ebc(const uint8_t *src,unsigned n,uint8_t *dst,unsigned cap);
int port110_ebc_to_asc(const uint8_t *src,unsigned n,uint8_t *dst,unsigned cap);
struct port110_ef_pool {uint64_t busy;};
void port110_ef_init(struct port110_ef_pool *p);
int port110_ef_get(struct port110_ef_pool *p,unsigned *number);
int port110_ef_free(struct port110_ef_pool *p,unsigned number);
int port110_ef_reserve(struct port110_ef_pool *p,unsigned number);
struct port110_lun_pool {uint32_t busy;};
void port110_lun_init(struct port110_lun_pool *p);
int port110_lun_get(struct port110_lun_pool *p,unsigned *number);
int port110_lun_free(struct port110_lun_pool *p,unsigned number);
/* Source-specific entry points tested in the host RTL suite. */
int port110_mthcexp(const uint16_t *a,uint16_t *out);
int port110_mthclog(const uint16_t *a,uint16_t *out);
int port110_mthcsqrt(const uint16_t *a,uint16_t *out);
int port110_mthcsinco_sin(const uint16_t *a,uint16_t *out);
int port110_mthcsinco_cos(const uint16_t *a,uint16_t *out);
int port110_mthcdexp(const uint16_t *a,uint16_t *out);
int port110_mthcdlog(const uint16_t *a,uint16_t *out);
int port110_mthcdsqrt(const uint16_t *a,uint16_t *out);
int port110_mthcdsinc_sin(const uint16_t *a,uint16_t *out);
int port110_mthcdsinc_cos(const uint16_t *a,uint16_t *out);
int port110_mthcgexp(const uint16_t *a,uint16_t *out);
int port110_mthcglog(const uint16_t *a,uint16_t *out);
int port110_mthcgsqrt(const uint16_t *a,uint16_t *out);
int port110_mthcgsinc_sin(const uint16_t *a,uint16_t *out);
int port110_mthcgsinc_cos(const uint16_t *a,uint16_t *out);
int port110_mthatanh(const uint16_t *a,uint16_t *out);
int port110_mthdatanh(const uint16_t *a,uint16_t *out);
int port110_mthgatanh(const uint16_t *a,uint16_t *out);
int port110_mthhatanh(const uint16_t *a,uint16_t *out);
int port110_mthsincos(const uint16_t *a,uint16_t *s,uint16_t *c);
int port110_mthsincosd(const uint16_t *a,uint16_t *s,uint16_t *c);
int port110_mthdsinco(const uint16_t *a,uint16_t *s,uint16_t *c);
int port110_mthdsincod(const uint16_t *a,uint16_t *s,uint16_t *c);
int port110_mthgsinco(const uint16_t *a,uint16_t *s,uint16_t *c);
int port110_mthgsincod(const uint16_t *a,uint16_t *s,uint16_t *c);
int port110_mthhsinco(const uint16_t *a,uint16_t *s,uint16_t *c);
int port110_mthhsincod(const uint16_t *a,uint16_t *s,uint16_t *c);
int port110_libpolyf(const uint16_t *x,unsigned order,const uint16_t *coeff,uint16_t *out);
int port110_libpolyd(const uint16_t *x,unsigned order,const uint16_t *coeff,uint16_t *out);
int port110_libpolyg(const uint16_t *x,unsigned order,const uint16_t *coeff,uint16_t *out);
int port110_libpolyh(const uint16_t *x,unsigned order,const uint16_t *coeff,uint16_t *out);
int port110_libemodf(const uint16_t *r,uint16_t ext,const uint16_t *m,int32_t *ip,uint16_t *fp);
int port110_libemodd(const uint16_t *r,uint16_t ext,const uint16_t *m,int32_t *ip,uint16_t *fp);
int port110_libemodg(const uint16_t *r,uint16_t ext,const uint16_t *m,int32_t *ip,uint16_t *fp);
int port110_libemodh(const uint16_t *r,uint16_t ext,const uint16_t *m,int32_t *ip,uint16_t *fp);
int port110_mthcvtdg_d_g(const uint16_t *a,uint16_t *out);
int port110_mthcvtdg_g_d(const uint16_t *a,uint16_t *out);
int port110_libtraa2e(const uint8_t *in,unsigned n,uint8_t *out,unsigned max);
int port110_libtrae2a(const uint8_t *in,unsigned n,uint8_t *out,unsigned max);
int port110_libef_get(struct port110_ef_pool *p,unsigned *n);
int port110_libef_free(struct port110_ef_pool *p,unsigned n);
int port110_libef_reserve(struct port110_ef_pool *p,unsigned n);
int port110_liblun_get(struct port110_lun_pool *p,unsigned *n);
int port110_liblun_free(struct port110_lun_pool *p,unsigned n);
int port110_mthsincos_sin(const uint16_t *a,uint16_t *out);
int port110_mthsincos_cos(const uint16_t *a,uint16_t *out);
int port110_mthsincos_sind(const uint16_t *a,uint16_t *out);
int port110_mthsincos_cosd(const uint16_t *a,uint16_t *out);
int port110_mthdsinco_sin(const uint16_t *a,uint16_t *out);
int port110_mthdsinco_cos(const uint16_t *a,uint16_t *out);
int port110_mthdsinco_sind(const uint16_t *a,uint16_t *out);
int port110_mthdsinco_cosd(const uint16_t *a,uint16_t *out);
int port110_mthgsinco_sin(const uint16_t *a,uint16_t *out);
int port110_mthgsinco_cos(const uint16_t *a,uint16_t *out);
int port110_mthgsinco_sind(const uint16_t *a,uint16_t *out);
int port110_mthgsinco_cosd(const uint16_t *a,uint16_t *out);
int port110_mthhsinco_sin(const uint16_t *a,uint16_t *out);
int port110_mthhsinco_cos(const uint16_t *a,uint16_t *out);
int port110_mthhsinco_sind(const uint16_t *a,uint16_t *out);
int port110_mthhsinco_cosd(const uint16_t *a,uint16_t *out);
int port110_mthconver_dfloti(int16_t value,uint16_t *out);
int port110_mthconver_dflotj(int32_t value,uint16_t *out);
int port110_mthconver_iint(const uint16_t *value,int32_t *out);
int port110_mthconver_jint(const uint16_t *value,int32_t *out);
int port110_mthconver_dble(const uint16_t *value,uint16_t *out);
int port110_mthconver_sngl(const uint16_t *value,uint16_t *out);
#endif
