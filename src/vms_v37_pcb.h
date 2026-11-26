#ifndef VMS_V37_PCB_H
#define VMS_V37_PCB_H
#include <stddef.h>
#include <stdint.h>

/* SYSDEF.MDL PCB byte offsets (packed VAX structure) and PDAT.MAR GENPCB.
 * The record preserves VMS fields; native processor context stays in PHD. */
enum {
    PORT_V37_PCB_BYTES=156u,
    PORT_V37_PCB_TYPE=12u,
    PORT_V37_PCB_SQFL=0u,PORT_V37_PCB_SQBL=4u,
    PORT_V37_PCB_SIZE=8u,PORT_V37_PCB_BTYPE=10u,
    PORT_V37_PCB_PRI=11u,PORT_V37_PCB_ASTEN=13u,
    PORT_V37_PCB_PRIB=47u,
    PORT_V37_PCB_ASTQFL=16u,PORT_V37_PCB_ASTQBL=20u,
    PORT_V37_PCB_PHYPCB=24u,PORT_V37_PCB_STS=36u,
    PORT_V37_PCB_WTIME=40u,PORT_V37_PCB_STATE=44u,
    PORT_V37_PCB_ASTCNT=56u,PORT_V37_PCB_BIOCNT=58u,
    PORT_V37_PCB_BIOLM=60u,PORT_V37_PCB_DIOCNT=62u,
    PORT_V37_PCB_DIOLM=64u,PORT_V37_PCB_EFCS=80u,
    PORT_V37_PCB_EFCU=84u,
    PORT_V37_PCB_EFWM=76u,PORT_V37_PCB_WEFC=46u,
    PORT_V37_PCB_EFC2P=88u,PORT_V37_PCB_EFC3P=92u,
    PORT_V37_PCB_STS_WALL=1u<<13,
    PORT_V37_PCB_PID=96u,PORT_V37_PCB_PHD=100u,
    PORT_V37_PCB_LNAME=104u,PORT_V37_PCB_PRIV=124u,
    PORT_V37_PCB_ARB=132u,PORT_V37_PCB_UIC=136u,
    PORT_V37_PCB_LOCKQFL=140u,PORT_V37_PCB_DLCKPRI=148u,
    PORT_V37_PCB_IPAST=152u,
    PORT_V37_PCB_STATE_CURRENT=14u,
    PORT_V37_PCB_STS_RESIDENT=1u<<0,
    PORT_V37_PCB_STS_PSWAPM=1u<<4,
    PORT_V37_PCB_STS_SSFEXCU=1u<<9,
    PORT_V37_PCB_STS_SSRWAIT=1u<<10,
    PORT_V37_PCB_STS_BATCH=1u<<14,
    PORT_V37_PCB_STS_NOACNT=1u<<15,
    PORT_V37_PCB_STS_HIBER=1u<<19,
    PORT_V37_PCB_STS_LOGIN=1u<<20,
    PORT_V37_PCB_STS_NETWRK=1u<<21,
    PORT_V37_PCB_STS_DISAWS=1u<<24
};
enum {PORT_V37_PCB_OK=1u,PORT_V37_PCB_BADPARAM=2u};

struct port_v37_pcb_seed {
    uint32_t address,pid,phd_address,phy_pcb_address,uic;
    uint8_t priority;
    const char *name;
    size_t name_bytes;
};

/* Emit PDAT.MAR's static GENPCB record, including its self-linked queue
 * heads, source priority encoding, DYN$C_PCB tag, privilege mask and inline
 * ARB pointer. A null PHD/PHYPCB denotes an unactivated PCB template. */
uint32_t port_v37_pcb_init_genpcb(uint8_t *,size_t,
    const struct port_v37_pcb_seed *);
uint32_t port_v37_pcb_init_syspcb_template(uint8_t *,size_t,uint32_t);
uint32_t port_v37_pcb_set_pix(uint8_t *,size_t,uint16_t);
/* Attach real native PHD and processor-context addresses to a source PCB. */
uint32_t port_v37_pcb_bind_context(uint8_t *,size_t,uint32_t phd_address,
    uint32_t native_phy_pcb_address);
/* INIT.MAR INI_SYSPCB: update the static PDAT system PCB after SYS PHD
 * creation. PID's high sequence word remains the GENPCB value of one. */
uint32_t port_v37_pcb_finish_syspcb(uint8_t *,size_t,uint16_t,
    uint32_t,uint32_t);
#endif
