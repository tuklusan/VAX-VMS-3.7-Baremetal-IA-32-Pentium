#include "vms_rtl85.h"

/* LIBANASTR.MAR: fixed, varying, and contiguous array descriptors. */
uint32_t port_lib_analyze_sdesc(const struct port_rtl_desc *d,
                                struct port_rtl_view *v) {
    if (!d || !v) return PORT_RTL_INVSTRDES;
    v->data=d->pointer;
    v->length=0;
    switch (d->class_code) {
    case PORT_RTL_CLASS_Z: case PORT_RTL_CLASS_S:
    case PORT_RTL_CLASS_D: case PORT_RTL_CLASS_SD:
        v->length=d->length;
        break;
    case PORT_RTL_CLASS_A: case PORT_RTL_CLASS_NCA:
        if (d->array_size>UINT16_MAX) return PORT_RTL_INVSTRDES;
        v->length=(uint16_t)d->array_size;
        break;
    case PORT_RTL_CLASS_VS:
        if (!v->data) return PORT_RTL_INVSTRDES;
        v->length=(uint16_t)v->data[0]|((uint16_t)v->data[1]<<8);
        v->data+=2;
        break;
    default:
        return PORT_RTL_INVSTRDES;
    }
    return (v->length && !v->data) ? PORT_RTL_INVSTRDES : PORT_RTL_NORMAL;
}

/* LIBLEN, LIBICHAR, LIBSKPC, LIBLOCC, LIBSCANC, LIBSPANC. */
uint32_t port_lib_len(const struct port_rtl_desc *d) {
    struct port_rtl_view v;
    return (port_lib_analyze_sdesc(d,&v)&1u)?v.length:0u;
}
uint32_t port_lib_ichar(const struct port_rtl_desc *d) {
    struct port_rtl_view v;
    return (port_lib_analyze_sdesc(d,&v)&1u) && v.length ? v.data[0]:0u;
}
uint32_t port_lib_skpc(const struct port_rtl_desc *ch,const struct port_rtl_desc *s) {
    struct port_rtl_view a,b;
    if (!(port_lib_analyze_sdesc(ch,&a)&1u) || !a.length ||
        !(port_lib_analyze_sdesc(s,&b)&1u)) return 0;
    for (uint32_t i=0;i<b.length;i++) if (b.data[i]!=a.data[0]) return i+1;
    return 0;
}
uint32_t port_lib_locc(const struct port_rtl_desc *ch,const struct port_rtl_desc *s) {
    struct port_rtl_view a,b;
    if (!(port_lib_analyze_sdesc(ch,&a)&1u) || !a.length ||
        !(port_lib_analyze_sdesc(s,&b)&1u)) return 0;
    for (uint32_t i=0;i<b.length;i++) if (b.data[i]==a.data[0]) return i+1;
    return 0;
}
uint32_t port_lib_scanc(const struct port_rtl_desc *s,const uint8_t t[256],uint8_t m) {
    struct port_rtl_view v;
    if (!t || !(port_lib_analyze_sdesc(s,&v)&1u)) return 0;
    for (uint32_t i=0;i<v.length;i++) if (t[v.data[i]] & m) return i+1;
    return 0;
}
uint32_t port_lib_spanc(const struct port_rtl_desc *s,const uint8_t t[256],uint8_t m) {
    struct port_rtl_view v;
    if (!t || !(port_lib_analyze_sdesc(s,&v)&1u)) return 0;
    for (uint32_t i=0;i<v.length;i++) if (!(t[v.data[i]] & m)) return i+1;
    return 0;
}

/* LIBINDEX, LIBMATCHC: find first substring, including the empty case. */
static uint32_t substring(const struct port_rtl_desc *s,const struct port_rtl_desc *sub) {
    struct port_rtl_view a,b;
    if (!(port_lib_analyze_sdesc(s,&a)&1u) ||
        !(port_lib_analyze_sdesc(sub,&b)&1u)) return 0;
    if (!b.length) return 1;
    if (b.length>a.length) return 0;
    for (uint32_t i=0;i<=((uint32_t)a.length-b.length);i++) {
        uint32_t j=0;
        while (j<b.length && a.data[i+j]==b.data[j]) j++;
        if (j==b.length) return i+1;
    }
    return 0;
}
uint32_t port_lib_index(const struct port_rtl_desc *s,const struct port_rtl_desc *sub){return substring(s,sub);}
uint32_t port_lib_matchc(const struct port_rtl_desc *sub,const struct port_rtl_desc *s){return substring(s,sub);}

/* LIBEXTZV, LIBEXTV, LIBINSV, LIBFFS, LIBFFC: byte-backed fields. */
static unsigned get_bit(const uint8_t *p,int64_t index){
    int64_t byte=index/8;
    int shift=(int)(index%8);
    if (shift<0){shift+=8;byte--;}
    return (p[byte]>>shift)&1u;
}
static void set_bit(uint8_t *p,int64_t index,unsigned bit){
    int64_t byte=index/8;
    int shift=(int)(index%8);
    if (shift<0){shift+=8;byte--;}
    uint8_t mask=(uint8_t)(1u<<shift);
    if(bit)p[byte]|=mask; else p[byte]&=(uint8_t)~mask;
}
uint32_t port_lib_extzv(int32_t pos,uint8_t size,const void *p,uint32_t *out){
    if(size>32) return PORT_RTL_RESERVED_OPERAND;
    if(!out || (!p && size)) return PORT_RTL_INVSTRDES;
    uint32_t value=0;
    for(unsigned i=0;i<size;i++) value |= get_bit(p,(int64_t)pos+i)<<i;
    *out=value;
    return PORT_RTL_NORMAL;
}
uint32_t port_lib_extv(int32_t pos,uint8_t size,const void *p,int32_t *out){
    if(!out) return PORT_RTL_INVSTRDES;
    uint32_t val=0,status=port_lib_extzv(pos,size,p,&val);
    if(!(status&1u)) return status;
    if(size && size<32 && (val&(1u<<(size-1)))) val |= UINT32_MAX<<size;
    *out=(int32_t)val;
    return status;
}
uint32_t port_lib_insv(uint32_t src,int32_t pos,uint8_t size,void *p){
    if(size>32)return PORT_RTL_RESERVED_OPERAND;
    if(!p && size)return PORT_RTL_INVSTRDES;
    for(unsigned i=0;i<size;i++)set_bit(p,(int64_t)pos+i,(src>>i)&1u);
    return PORT_RTL_NORMAL;
}
static uint32_t find_bit(uint32_t pos,uint8_t n,const void *p,uint32_t *result,unsigned desired){
    if(n>32)return PORT_RTL_RESERVED_OPERAND;
    if(n && (!p || !result))return PORT_RTL_INVSTRDES;
    for(unsigned i=0;i<n;i++){
        if(get_bit(p,(int64_t)pos+i)==desired){*result=pos+i;return PORT_RTL_NORMAL;}
    }
    return PORT_RTL_NOTFOU;
}
uint32_t port_lib_ffs(uint32_t pos,uint8_t n,const void *p,uint32_t *result){return find_bit(pos,n,p,result,1u);}
uint32_t port_lib_ffc(uint32_t pos,uint8_t n,const void *p,uint32_t *result){return find_bit(pos,n,p,result,0u);}

/* LIBMOVC3, LIBMOVC5: copy in overlap-safe byte order. */
uint32_t port_lib_movc3(uint16_t n,const void *s,void *d){
    if(n && (!s||!d))return PORT_RTL_INVSTRDES;
    const uint8_t *a=s;
    uint8_t *b=d;
    if(n && (uintptr_t)b>(uintptr_t)a &&
       (uintptr_t)b-(uintptr_t)a<n){
        for(uint32_t i=n;i>0;i--)b[i-1]=a[i-1];
    }else{
        for(uint32_t i=0;i<n;i++)b[i]=a[i];
    }
    return PORT_RTL_NORMAL;
}
uint32_t port_lib_movc5(uint16_t ns,const void *s,uint8_t fill,uint16_t nd,void *d){
    if(nd && !d)return PORT_RTL_INVSTRDES;
    uint16_t n=ns<nd?ns:nd;
    uint32_t st=port_lib_movc3(n,s,d);
    if(!(st&1u))return st;
    uint8_t *b=d;
    for(uint32_t i=n;i<nd;i++)b[i]=fill;
    return PORT_RTL_NORMAL;
}

/* SYS/SRC/CVTATB.MAR: unsigned 32-bit decimal, octal and hex parsing. */
static int cvt_tb(uint32_t n,const uint8_t *s,uint32_t *out,uint32_t radix){
    if(!n || !s || !out)return 0;
    uint32_t v=0,negative=0,start=0;
    if(s[0]=='+' || s[0]=='-'){negative=s[0]=='-';start=1;}
    for(uint32_t i=start;i<n;i++){
        unsigned c=s[i],digit;
        if(c>='0'&&c<='9') digit=c-'0';
        else if(c>='A'&&c<='Z') digit=c-'A'+10u;
        else if(c>='a'&&c<='z') digit=c-'a'+10u;
        else return 0;
        if(digit>=radix || v>(UINT32_MAX-digit)/radix)return 0;
        v=v*radix+digit;
    }
    *out=negative ? 0u-v:v;
    return 1;
}
int port_sys_cvt_dtb(uint32_t n,const uint8_t *s,uint32_t *out){return cvt_tb(n,s,out,10u);}
int port_sys_cvt_otb(uint32_t n,const uint8_t *s,uint32_t *out){return cvt_tb(n,s,out,8u);}
int port_sys_cvt_htb(uint32_t n,const uint8_t *s,uint32_t *out){return cvt_tb(n,s,out,16u);}
