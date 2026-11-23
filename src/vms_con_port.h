#ifndef VMS_CON_PORT_H
#define VMS_CON_PORT_H
#include <stddef.h>
#include <stdint.h>
/* Source: SYS/SRC/CONINTDSP.MAR 484-550 and SHRLIB/TTYDEF.MAR 245-449.
 * These act on original VMS 3.7 UCB bytes, not on a VAX emulator. The
 * class routine TERMINALIO and real OP/TT DPTs have not been linked.
 * DEFERRED means the caller must not claim an output was transmitted. */
enum {
    PORT_V37_CON_OK=1u, PORT_V37_CON_BAD=2u, PORT_V37_CON_DEFERRED=4u,
    PORT_V37_CON_UCB_BYTES=256u,
    PORT_V37_CON_STS=88u, PORT_V37_CON_DUETIM=92u,
    PORT_V37_CON_STATE=132u, PORT_V37_CON_HOLD=174u,
    PORT_V37_CON_OUTLEN=220u,
    PORT_V37_CON_TIM=1u, PORT_V37_CON_INT=2u,
    PORT_V37_CON_CTRLS=1u,
    PORT_V37_CON_XOFF=0x100u, PORT_V37_CON_XON=0x200u,
    PORT_V37_CON_STOP=0x400u, PORT_V37_CON_STOP2=0x800u,
    PORT_V37_CON_BURST=0x2000u
};
struct port_v37_con_backend {
    /* Real terminal class/port dispatch; no simulated success on NULL. */
    uint32_t (*kick)(void *);
    void *context;
};
uint16_t port_v37_con_get16(const uint8_t *,unsigned);
uint32_t port_v37_con_get32(const uint8_t *,unsigned);
void port_v37_con_set16(uint8_t *,unsigned,uint16_t);
void port_v37_con_set32(uint8_t *,unsigned,uint32_t);
uint32_t port_v37_con_valid(const uint8_t *,size_t);
uint32_t port_v37_con_kick(uint8_t *,const struct port_v37_con_backend *);
uint32_t port_v37_con_xoff(uint8_t *,size_t,const struct port_v37_con_backend *);
uint32_t port_v37_con_xon(uint8_t *,size_t,const struct port_v37_con_backend *);
uint32_t port_v37_con_stop(uint8_t *,size_t);
uint32_t port_v37_con_stop2(uint8_t *,size_t,uint32_t);
uint32_t port_v37_con_abort(uint8_t *,size_t,uint32_t);
uint32_t port_v37_con_resume(uint8_t *,size_t,uint32_t,const struct port_v37_con_backend *);
#endif
