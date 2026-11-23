#ifndef VMS_INI_TTYDRV_H
#define VMS_INI_TTYDRV_H
#include <stddef.h>
#include <stdint.h>
/* Native counterpart of SYS/SRC/INIT.MAR INI_TTYDRV 1256-1304.
 * DPT field offsets are defined by SHRLIB/SYSMAR.MAR's DPTAB (56 bytes);
 * class vector slots by SHRLIB/TTYDEF.MAR (6 longwords followed by zero)
 * and actual source TTDRVR/SRC/TTYDRVDAT.MAR CLASS_VECTOR.
 * This transforms a REAL loaded class-driver image. It does not supply
 * missing TTDRIVER bytecode or initialize real OPA0 I/O by itself. */
enum {
    PORT_INI_TTY_OK=1u, PORT_INI_TTY_BADPARAM=2u,
    PORT_INI_TTY_BAD_DPT=4u, PORT_INI_TTY_TOO_SMALL_UCB=6u,
    PORT_INI_TTY_BAD_VECTOR=8u
};
struct port_ini_tty_class {
    uint32_t dpt_address;
    uint32_t vector_address;
    uint32_t ddt_address;       /* CLASS_DDT: entry 4 */
    uint32_t getnext_address;   /* CLASS_GETNXT: entry 0 */
    uint32_t putnext_address;   /* CLASS_PUTNXT: entry 1 */
    uint32_t relocated_entries;
    uint16_t required_console_ucb_bytes;
};
uint32_t port_ini_tty_class_relocate(uint8_t *image,size_t image_capacity,
    uint32_t guest_image_base,size_t dpt_offset,uint8_t dpt_type_code,
    uint16_t console_ucb_bytes,struct port_ini_tty_class *out);
/* INIT.MAR 1289-1304 attaches the relocated TT class and the prelinked
 * OP$DPT port vector to the actual OPA0 DDB/UCB. All offsets are sourced
 * from DEVICEDAT.MAR and SHRLIB/TTYDEF.MAR; no terminal I/O is performed. */
struct port_ini_tty_console_binding {
    uint32_t class_ddt_address;
    uint32_t port_vector_address;
};
uint32_t port_ini_tty_bind_console(uint8_t *ddb,size_t ddb_capacity,
    uint8_t *console_ucb,size_t ucb_capacity,
    const uint8_t *op_dpt,size_t op_dpt_capacity,uint32_t op_dpt_address,
    uint8_t expected_dpt_type,
    const struct port_ini_tty_class *ttyclass,
    struct port_ini_tty_console_binding *out);
#endif
