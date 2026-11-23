#include "vms_ini_ttydrv.h"
/* 32-bit VAX descriptors and native IA-32 pointer words share the same
 * byte width; the routine executes straight IA-32 address translation,
 * NOT a VAX instruction emulator. */
static uint16_t u16(const uint8_t *p){
    return (uint16_t)p[0]|((uint16_t)p[1]<<8);
}
static uint32_t u32(const uint8_t *p){
    return (uint32_t)p[0]|((uint32_t)p[1]<<8)|
           ((uint32_t)p[2]<<16)|((uint32_t)p[3]<<24);
}
static void set32(uint8_t *p,uint32_t v){
    p[0]=(uint8_t)v;p[1]=(uint8_t)(v>>8);
    p[2]=(uint8_t)(v>>16);p[3]=(uint8_t)(v>>24);
}
uint32_t port_ini_tty_class_relocate(uint8_t *image,size_t image_capacity,
    uint32_t guest_image_base,size_t dpt_offset,uint8_t dpt_type_code,
    uint16_t console_ucb_bytes,struct port_ini_tty_class *out){
    /* DPTAB offsets are proven from original macro expansion:
     * +8 size, +10 type, +14 UCB size, +30 relative vector offset. */
    if(!image||!out || !guest_image_base ||
       image_capacity<56u || image_capacity>UINT32_MAX ||
       (uintptr_t)image&3u || (guest_image_base&3u) ||
       (dpt_offset&3u) || dpt_offset>image_capacity-56u ||
       (uint64_t)guest_image_base+image_capacity>UINT64_C(0x100000000))
        return PORT_INI_TTY_BADPARAM;
    uint8_t *dpt=image+dpt_offset;
    uint16_t declared_bytes=u16(dpt+8u);
    if(dpt[10u]!=dpt_type_code || declared_bytes<56u ||
       declared_bytes>image_capacity-dpt_offset)
        return PORT_INI_TTY_BAD_DPT;
    uint16_t ucb_required=u16(dpt+14u);
    if(console_ucb_bytes<ucb_required)
        return PORT_INI_TTY_TOO_SMALL_UCB;
    uint32_t vec_dpt=u16(dpt+30u);
    if(vec_dpt<56u || (vec_dpt&3u) ||
       vec_dpt>image_capacity-dpt_offset-28u ||
       vec_dpt>(uint32_t)declared_bytes-28u)
        return PORT_INI_TTY_BAD_VECTOR;
    uint8_t *vec=dpt+vec_dpt;
    uint32_t count=0u;
    size_t limit=(image_capacity-dpt_offset-vec_dpt)/4u;
    /* Original six CLASS_* entries, followed by a zero terminator.
     * The old code loops over all nonzero entries and leaves zero intact.
     * No partial relocation if a malformed driver lacks that sentinel. */
    for(;count<limit;count++){
        if(u32(vec+4u*count)==0u)break;
    }
    if(count<6u || count>=limit)
        return PORT_INI_TTY_BAD_VECTOR;
    uint32_t base=guest_image_base+(uint32_t)dpt_offset;
    for(uint32_t i=0;i<count;i++){
        uint8_t *cell=vec+4u*i;
        /* Source ADDL2 R2,(R1)+ relocates a 32-bit word, wrapping exactly as
         * the native 32-bit instruction when an offset is negative. */
        set32(cell,u32(cell)+base);
    }
    *out=(struct port_ini_tty_class){
      .dpt_address=base,.vector_address=base+vec_dpt,
      .ddt_address=u32(vec+16u),
      .getnext_address=u32(vec),.putnext_address=u32(vec+4u),
      .relocated_entries=count,.required_console_ucb_bytes=ucb_required};
    return PORT_INI_TTY_OK;
}

uint32_t port_ini_tty_bind_console(uint8_t *ddb,size_t ddb_capacity,
    uint8_t *console_ucb,size_t ucb_capacity,
    const uint8_t *op_dpt,size_t op_dpt_capacity,uint32_t op_dpt_address,
    uint8_t expected_dpt_type,
    const struct port_ini_tty_class *ttyclass,
    struct port_ini_tty_console_binding *out){
    /* DEVICEDAT.MAR: DDB$L_DDT = 12, UCB$L_DDT = 120. UCB size
     * and vector offsets derive from TTYDEF.MAR after standard 132-byte
     * UCB: GETNXT=200, PUTNXT=204, CLASS=208, PORT=212. */
    enum {DDB_DDT=12u,UCB_DDT=120u,UCB_GETNXT=200u,
          UCB_PUTNXT=204u,UCB_CLASS=208u,UCB_PORT=212u};
    if(!ddb||!console_ucb||!op_dpt||!ttyclass||!out ||
       ddb_capacity<DDB_DDT+4u || ucb_capacity<UCB_PORT+4u ||
       op_dpt_capacity<56u || !op_dpt_address || (op_dpt_address&3u) ||
       !ttyclass->ddt_address || !ttyclass->vector_address ||
       !ttyclass->getnext_address || !ttyclass->putnext_address ||
       ttyclass->relocated_entries<6u ||
       (uintptr_t)ddb&3u || (uintptr_t)console_ucb&3u ||
       (uintptr_t)op_dpt&3u)
        return PORT_INI_TTY_BADPARAM;
    if(op_dpt[10u]!=expected_dpt_type || u16(op_dpt+8u)<56u)
        return PORT_INI_TTY_BAD_DPT;
    uint16_t console_size=u16(console_ucb+8u);
    if(console_size<ttyclass->required_console_ucb_bytes ||
       console_size>ucb_capacity || console_size<UCB_PORT+4u)
        return PORT_INI_TTY_TOO_SMALL_UCB;
    uint16_t port_offset=u16(op_dpt+30u);
    if(!port_offset || (port_offset&3u) ||
       port_offset>UINT32_MAX-op_dpt_address)
        return PORT_INI_TTY_BAD_VECTOR;
    const uint32_t port_vector=op_dpt_address+port_offset;
    /* Source writes the new class DDT to both DDB and UCB, and the
     * GETNXT/PUTNXT/class/port addresses to the same OPA0 UCB. */
    set32(ddb+DDB_DDT,ttyclass->ddt_address);
    set32(console_ucb+UCB_DDT,ttyclass->ddt_address);
    set32(console_ucb+UCB_GETNXT,ttyclass->getnext_address);
    set32(console_ucb+UCB_PUTNXT,ttyclass->putnext_address);
    set32(console_ucb+UCB_CLASS,ttyclass->vector_address);
    set32(console_ucb+UCB_PORT,port_vector);
    *out=(struct port_ini_tty_console_binding){
        .class_ddt_address=ttyclass->ddt_address,
        .port_vector_address=port_vector};
    return PORT_INI_TTY_OK;
}
