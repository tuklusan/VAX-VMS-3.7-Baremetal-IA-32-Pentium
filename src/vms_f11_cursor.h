#ifndef VMS37_F11_CURSOR_H
#define VMS37_F11_CURSOR_H
#include "vms_f11_chain.h"
/* SYS/SRC/FILEREAD.MAR FIL$READVBN, GETRTRVPTR and FIL$RDWRTLBN.
 * Privileged, read-only Files-11 byte position over checked FH2 extent
 * chains. Not a VMS channel, ACP, AST, $QIO, or user-mode file handle. */
enum { PORT_F11_CURSOR_OK=1u, PORT_F11_CURSOR_BAD=2u,
       PORT_F11_CURSOR_EOF=4u, PORT_F11_CURSOR_MAP=6u,
       PORT_F11_CURSOR_IO=8u, PORT_F11_CURSOR_DIFFERENT=10u };
struct port_f11_cursor {
    const struct port_f11_boot_volume *volume;
    const struct port_f11_chain *chain;
    uint32_t position, total_bytes, reads;
};
uint32_t port_f11_cursor_init(struct port_f11_cursor *,
    const struct port_f11_boot_volume *,const struct port_f11_chain *,uint32_t);
uint32_t port_f11_cursor_seek(struct port_f11_cursor *,uint32_t);
uint32_t port_f11_cursor_skip(struct port_f11_cursor *,uint32_t);
uint32_t port_f11_cursor_locate(const struct port_f11_cursor *,
    uint32_t *,uint32_t *);
uint32_t port_f11_cursor_read(struct port_f11_cursor *,uint8_t *,uint32_t,
    size_t,uint32_t *);
uint32_t port_f11_cursor_word(struct port_f11_cursor *,uint16_t *);
uint32_t port_f11_cursor_long(struct port_f11_cursor *,uint32_t *);
uint32_t port_f11_cursor_compare(struct port_f11_cursor *,
    const uint8_t *,uint32_t,uint32_t *);
#endif
