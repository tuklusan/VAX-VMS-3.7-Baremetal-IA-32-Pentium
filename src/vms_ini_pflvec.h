#ifndef VMS_INI_PFLVEC_H
#define VMS_INI_PFLVEC_H
#include <stdint.h>
#include <stddef.h>
/* Partial source counterpart for SYS/SRC/INIT.MAR INI_MISC, INI_PFLVEC
 * and INI_INTSTKLM. Builds the original page/swap file pointer record
 * and per-IPL interrupt stack limits; does not activate the page manager
 * or the executive's real lock manager. A zero shell base/window pair
 * explicitly leaves the shell VBN/window unbound (zero) until they exist. */
enum {PORT_INI_PFL_OK=1u,PORT_INI_PFL_BADPARAM=2u,
      PORT_INI_PFL_NOMEM=4u};
struct port_ini_pflvec_settings {
    uint32_t page_file_count, swap_file_count;
    uint32_t null_pfl_address;
    uint16_t ptr_dyn_type, pfl_dyn_type;
    uint32_t global_page_file_limit;
    uint32_t shell_base_address, shell_system_window;
};
struct port_ini_pflvec {
    const uint8_t *pointers;
    uint32_t entries, allocated_bytes;
    uint32_t null_pagefile_vbn, null_pagefile_window;
    uint32_t actual_swapfile_slots;
    uint32_t global_page_file_limit;
};
struct port_ini_interrupt_stack {
    uint32_t low,high;
    uint8_t max_resource_depth;
};
uint32_t port_ini_pflvec_init(void *memory,size_t capacity,
                const struct port_ini_pflvec_settings *settings,
                struct port_ini_pflvec *result);
uint32_t port_ini_pflvec_entry(const struct port_ini_pflvec *s,uint32_t i);
uint32_t port_ini_interrupt_stack_limits(uint32_t high,
                uint32_t stack_pages,uint32_t extra_stack_bytes,
                struct port_ini_interrupt_stack *out);
#endif
