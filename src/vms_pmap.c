#include "vms_pmap.h"
/* Physical-memory locations and page-table input are trusted kernel data. */
void port_pmap_user_range(uint32_t table[PORT_PMAP_ENTRIES],
                          uintptr_t begin, uintptr_t end, int writable) {
    if (!table || begin >= end || end > 0x400000u) return;
    uintptr_t start=begin & ~(uintptr_t)(PORT_PAGE_BYTES-1);
    uintptr_t limit=(end+PORT_PAGE_BYTES-1) & ~(uintptr_t)(PORT_PAGE_BYTES-1);
    for (uintptr_t addr=start;addr<limit;addr+=PORT_PAGE_BYTES) {
        unsigned page=(unsigned)(addr/PORT_PAGE_BYTES);
        table[page]=((uint32_t)page<<12) | 1u | 4u | (writable ? 2u:0u);
    }
}
void port_pmap_task(uint32_t table[PORT_PMAP_ENTRIES],
                    uintptr_t text_begin, uintptr_t text_end,
                    uintptr_t stack_begin, uintptr_t stack_end,
                    uintptr_t clock_begin, uintptr_t clock_end) {
    if (!table) return;
    for (unsigned i=0;i<PORT_PMAP_ENTRIES;i++) table[i]=(i<<12)|3u;
    port_pmap_user_range(table,text_begin,text_end,0);
    port_pmap_user_range(table,stack_begin,stack_end,1);
    port_pmap_user_range(table,clock_begin,clock_end,0);
}

void port_pmap_guard_page(uint32_t table[PORT_PMAP_ENTRIES], uintptr_t guard) {
    if (!table || (guard & (PORT_PAGE_BYTES-1u)) ||
        guard>=0x400000u) return;
    table[guard/PORT_PAGE_BYTES]=0;
}
