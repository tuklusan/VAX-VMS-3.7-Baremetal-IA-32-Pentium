#ifndef VMS_RTL_M107_H
#define VMS_RTL_M107_H
/* Native IA-32 helpers for source-specific VMS 3.7 RTL routines.
 * Raw VAX F/D/G/H are represented as 16-bit words in original byte order;
 * they are never cast to host IEEE floating-point types. */
#include <stddef.h>
#include <stdint.h>
enum port_m107_fmt { PORT_M107_F=2, PORT_M107_D=4, PORT_M107_G=5, PORT_M107_H=8 };
enum port_m107_result { PORT_M107_OK=1, PORT_M107_BAD_ARG=2,
 PORT_M107_RESERVED_FLOAT=4, PORT_M107_OVERFLOW=6 };
struct port_m107_desc { const uint8_t *data; size_t bytes; };
int port_m107_float_valid(enum port_m107_fmt fmt,const uint16_t *v);
int port_m107_float_cmp(enum port_m107_fmt fmt,const uint16_t *a,const uint16_t *b,int *relation);
int port_m107_float_extreme(enum port_m107_fmt fmt,const uint16_t * const *args,size_t count,uint16_t *out,int take_max);
int port_m107_float_sign(enum port_m107_fmt fmt,const uint16_t *arg,const uint16_t *sign,uint16_t *out);
int port_m107_float_conj(enum port_m107_fmt fmt,const uint16_t *pair,uint16_t *out);
int port_m107_float_rounded(enum port_m107_fmt fmt,const uint16_t *v,unsigned outbits,int32_t *out);
int port_m107_int16_extreme(const int16_t * const *a,size_t n,int max,int16_t *out);
int port_m107_int32_extreme(const int32_t * const *a,size_t n,int max,int32_t *out);
int port_m107_int16_sign(int16_t mag,int16_t sign,int16_t *out);
int port_m107_int32_sign(int32_t mag,int32_t sign,int32_t *out);
int port_m107_rad50_word(const char *ascii,size_t n,uint16_t *out,size_t *used);
int port_m107_rad50_six(const char *ascii,uint32_t *out);
int port_m107_irad50(const char *ascii,size_t length,uint16_t *output,size_t capacity,size_t *consumed);
int port_m107_r50asc(const uint16_t *encoded,size_t count,char *ascii,size_t capacity);
/* One function per original source file. Symbols are generated source-by-source. */
int port_m107_mthiidnnt(const uint16_t *a,int32_t *r);
int port_m107_mthiignnt(const uint16_t *a,int32_t *r);
int port_m107_mthiihnnt(const uint16_t *a,int32_t *r);
int port_m107_mthjidnnt(const uint16_t *a,int32_t *r);
int port_m107_mthjignnt(const uint16_t *a,int32_t *r);
int port_m107_mthjihnnt(const uint16_t *a,int32_t *r);
int port_m107_mthinint(const uint16_t *a,int32_t *r);
int port_m107_mthjnint(const uint16_t *a,int32_t *r);
int port_m107_mthiisign(int16_t magnitude,int16_t sign,int16_t *out);
int port_m107_mthjisign(int32_t magnitude,int32_t sign,int32_t *out);
int port_m107_mthimax0(const int16_t * const *a,size_t n,int16_t *out);
int port_m107_mthimin0(const int16_t * const *a,size_t n,int16_t *out);
int port_m107_mthjmax0(const int32_t * const *a,size_t n,int32_t *out);
int port_m107_mthjmin0(const int32_t * const *a,size_t n,int32_t *out);
int port_m107_mthsign(const uint16_t *a,const uint16_t *b,uint16_t *out);
int port_m107_mthdsign(const uint16_t *a,const uint16_t *b,uint16_t *out);
int port_m107_mthgsign(const uint16_t *a,const uint16_t *b,uint16_t *out);
int port_m107_mthhsign(const uint16_t *a,const uint16_t *b,uint16_t *out);
int port_m107_mthmax1(const uint16_t * const *a,size_t n,uint16_t *out);
int port_m107_mthmin1(const uint16_t * const *a,size_t n,uint16_t *out);
int port_m107_mthdmax1(const uint16_t * const *a,size_t n,uint16_t *out);
int port_m107_mthdmin1(const uint16_t * const *a,size_t n,uint16_t *out);
int port_m107_mthgmax1(const uint16_t * const *a,size_t n,uint16_t *out);
int port_m107_mthgmin1(const uint16_t * const *a,size_t n,uint16_t *out);
int port_m107_mthhmax1(const uint16_t * const *a,size_t n,uint16_t *out);
int port_m107_mthhmin1(const uint16_t * const *a,size_t n,uint16_t *out);
int port_m107_mthconjg(const uint16_t *pair,uint16_t *out);
int port_m107_mthdconjg(const uint16_t *pair,uint16_t *out);
int port_m107_mthgconjg(const uint16_t *pair,uint16_t *out);
int port_m107_mthsgn(const uint16_t *v,int *result);
int port_m107_comr50wd(const char *ascii,size_t n,uint16_t *out,size_t *used);
int port_m107_comrad50(const char *ascii,uint32_t *out);
int port_m107_comirad50(const char *ascii,size_t n,uint16_t *out,size_t capacity,size_t *consumed);
int port_m107_comr50asc(const uint16_t *encoded,size_t n,char *ascii,size_t capacity);
#endif
