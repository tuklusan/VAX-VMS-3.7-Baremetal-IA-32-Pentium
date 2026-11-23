#include "vms_ini_dpt_templates.h"
/* Original DPTAB macro's fixed 56-byte structure. DPT$W_SIZE denotes
 * the WHOLE linked driver, not this 56-byte prefix. It must remain zero
 * here; these records MUST NOT join IOC$GL_DPTLIST as active DPTs. */
static void w16(uint8_t *p,uint16_t x){p[0]=(uint8_t)x;p[1]=(uint8_t)(x>>8);}
static uint16_t r16(const uint8_t *p){return (uint16_t)((unsigned)p[0]|((unsigned)p[1]<<8));}
uint32_t port_v37_dpt_fill(uint8_t *r,size_t cap,uint16_t ucb,
                          const char *name,unsigned len){
    if(!r || !name || cap<PORT_V37_DPT_STAGE_BYTES || len==0u || len>11u || !ucb)
        return 0u;
    for(unsigned i=0;i<PORT_V37_DPT_STAGE_BYTES;i++)r[i]=0u;
    /* 0-7 FLINK,BLINK: deliberately empty until driver executable. */
    /* 8-9 SIZE: 0 means unavailable driver END / not publishable. */
    r[10]=PORT_V37_DYN_DPT;
    r[12]=PORT_V37_DPT_AT_UBA; /* source UBA literal, NOT PC hardware */
    w16(r+14u,ucb);
    w16(r+16u,PORT_V37_DPT_PREFIX_BYTES);
    w16(r+18u,PORT_V37_DPT_PREFIX_BYTES);
    w16(r+22u,PORT_V37_DPT_MAXUNITS);
    w16(r+24u,PORT_V37_DPT_VERSION);
    w16(r+26u,PORT_V37_DPT_DEFUNITS);
    r[32]=(uint8_t)len;
    for(unsigned i=0;i<len;i++)r[33u+i]=(uint8_t)name[i];
    /* Empty DPT_STORE INIT,REINIT,END: terminator at 56. */
    return 1u;
}
static int match(const uint8_t *p,uint16_t ucb,const char *name,unsigned len){
    if(r16(p+8u)!=0u || p[10]!=PORT_V37_DYN_DPT || p[11]!=0u ||
       p[12]!=PORT_V37_DPT_AT_UBA || p[13]!=0u ||
       r16(p+14u)!=ucb || r16(p+16u)!=PORT_V37_DPT_PREFIX_BYTES ||
       r16(p+18u)!=PORT_V37_DPT_PREFIX_BYTES || r16(p+20u)!=0u ||
       r16(p+22u)!=PORT_V37_DPT_MAXUNITS ||
       r16(p+24u)!=PORT_V37_DPT_VERSION ||
       r16(p+26u)!=PORT_V37_DPT_DEFUNITS ||
       r16(p+28u)!=0u || r16(p+30u)!=0u ||p[32u]!=len)
        return 0;
    for(unsigned i=0;i<8u;i++)if(p[i]!=0u)return 0;
    for(unsigned i=0;i<len;i++)if(p[33u+i]!=(uint8_t)name[i])return 0;
    for(unsigned i=33u+len;i<PORT_V37_DPT_STAGE_BYTES;i++)if(p[i]!=0u)return 0;
    return 1;
}
uint32_t port_v37_dpt_validate_staged(const uint8_t rows[3][PORT_V37_DPT_STAGE_BYTES]){
    return rows && match(rows[0],PORT_V37_DPT_MB_UCBSIZE,"MBDRIVER",8u) &&
         match(rows[1],PORT_V37_DPT_NL_UCBSIZE,"NLDRIVER",8u) &&
         match(rows[2],PORT_V37_DPT_OP_UCBSIZE,"OPERATOR",8u);
}
