#ifndef VMS_INI_SYSWCB_H
#define VMS_INI_SYSWCB_H
#include <stddef.h>
#include <stdint.h>
/* SYS/SRC/INIT.MAR, INI_SYSPCB, BOO$L_SYS_MAP to SYSWCB transformation.
 * A BOO$L_SYS_MAP starts with the byte length of an array of normalized
 * {32-bit block count, 32-bit LBN} retrieval pointers. Original INIT
 * compacts each to a 6-byte {16-bit count, 32-bit LBN} pointer in place,
 * shifts them after SYSWCB and overlays the linked WCB template.
 * This routine does not create a functioning WCB, file-cache service, or
 * guest process: the original SYSWCB template and boot CB still have to be
 * wired into resident SYS.EXE before guest execution can be claimed. */
enum {
    PORT_INI_SYSWCB_OK=1u,
    PORT_INI_SYSWCB_BADPARAM=2u,
    PORT_INI_SYSWCB_NOMEM=4u,
    PORT_INI_SYSWCB_BADMAP=6u
};
struct port_ini_syswcb_layout {
    uint32_t header_length; /* WCB$C_LENGTH */
    uint32_t size_word_offset; /* WCB$W_SIZE */
    uint32_t map_count_word_offset; /* WCB$W_NMAP */
};
struct port_ini_syswcb {
    const uint8_t *record;
    const uint8_t *retrieval_pointers;
    uint16_t mapping_count;
    uint16_t record_size_field;
    uint32_t record_length;
};
uint32_t port_ini_syswcb_from_boot_map(
    uint8_t *boot_map, size_t boot_map_capacity,
    const uint8_t *syswcb_template, size_t template_capacity,
    const struct port_ini_syswcb_layout *layout,
    struct port_ini_syswcb *result);
uint32_t port_ini_syswcb_retrieval(const struct port_ini_syswcb *wcb,
    uint32_t index, uint16_t *blocks, uint32_t *starting_lbn);
#endif
