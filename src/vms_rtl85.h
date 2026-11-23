#ifndef VMS_RTL85_H
#define VMS_RTL85_H
#include <stdint.h>
#include <stddef.h>

/* Native counterparts for VMS 3.7 RTL/SRC string and bit services. */
enum { PORT_RTL_NORMAL=1u, PORT_RTL_INVSTRDES=1409572u,
       PORT_RTL_NOTFOU=1409652u, PORT_RTL_RESERVED_OPERAND=0xFFFFFFFEu };
enum { PORT_RTL_CLASS_Z=0, PORT_RTL_CLASS_S=1, PORT_RTL_CLASS_D=2,
       PORT_RTL_CLASS_A=4, PORT_RTL_CLASS_SD=9, PORT_RTL_CLASS_NCA=10,
       PORT_RTL_CLASS_VS=11 };
struct port_rtl_desc {
    uint16_t length;
    uint8_t dtype;
    uint8_t class_code;
    const uint8_t *pointer;
    uint32_t array_size;
};
struct port_rtl_view { const uint8_t *data; uint16_t length; };
uint32_t port_lib_analyze_sdesc(const struct port_rtl_desc *,struct port_rtl_view *);
uint32_t port_lib_len(const struct port_rtl_desc *);
uint32_t port_lib_ichar(const struct port_rtl_desc *);
uint32_t port_lib_skpc(const struct port_rtl_desc *,const struct port_rtl_desc *);
uint32_t port_lib_locc(const struct port_rtl_desc *,const struct port_rtl_desc *);
uint32_t port_lib_scanc(const struct port_rtl_desc *,const uint8_t[256],uint8_t);
uint32_t port_lib_spanc(const struct port_rtl_desc *,const uint8_t[256],uint8_t);
uint32_t port_lib_index(const struct port_rtl_desc *,const struct port_rtl_desc *);
uint32_t port_lib_matchc(const struct port_rtl_desc *,const struct port_rtl_desc *);
uint32_t port_lib_extzv(int32_t,uint8_t,const void *,uint32_t *);
uint32_t port_lib_extv(int32_t,uint8_t,const void *,int32_t *);
uint32_t port_lib_insv(uint32_t,int32_t,uint8_t,void *);
uint32_t port_lib_ffs(uint32_t,uint8_t,const void *,uint32_t *);
uint32_t port_lib_ffc(uint32_t,uint8_t,const void *,uint32_t *);
uint32_t port_lib_movc3(uint16_t,const void *,void *);
uint32_t port_lib_movc5(uint16_t,const void *,uint8_t,uint16_t,void *);
int port_sys_cvt_dtb(uint32_t,const uint8_t *,uint32_t *);
int port_sys_cvt_otb(uint32_t,const uint8_t *,uint32_t *);
int port_sys_cvt_htb(uint32_t,const uint8_t *,uint32_t *);
extern const uint8_t port_lib_ab_upcase[256];
extern const uint8_t port_lib_ab_cvt_u_o[20];
extern const uint8_t port_lib_ab_cvt_o_u[256];
extern const uint8_t port_lib_ab_cvtpt_u[256];
extern const uint8_t port_lib_ab_cvttp_u[256];
extern const uint8_t port_lib_ab_cvtpt_o[256];
extern const uint8_t port_lib_ab_cvttp_o[256];
extern const uint8_t port_lib_ab_cvtpt_z[256];
extern const uint8_t port_lib_ab_cvttp_z[256];
#endif
