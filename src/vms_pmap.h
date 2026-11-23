#ifndef PORTLAB_VMS_PMAP_H
#define PORTLAB_VMS_PMAP_H
#include <stdint.h>
#include <stddef.h>
/* First 4 MiB: flat supervisor identity mapping with a narrow user aperture.
 * This is an IA-32 laboratory page-table builder, not VAX's 512-byte PTE. */
enum { PORT_PMAP_ENTRIES=1024, PORT_PAGE_BYTES=4096 };
void port_pmap_user_range(uint32_t table[PORT_PMAP_ENTRIES],
                          uintptr_t begin, uintptr_t end, int writable);
/* Kernel guard page: no present, user, or writable bits. */
void port_pmap_guard_page(uint32_t table[PORT_PMAP_ENTRIES], uintptr_t guard);
void port_pmap_task(uint32_t table[PORT_PMAP_ENTRIES],
                    uintptr_t text_begin, uintptr_t text_end,
                    uintptr_t stack_begin, uintptr_t stack_end,
                    uintptr_t clock_begin, uintptr_t clock_end);
#endif
