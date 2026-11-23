#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include "vms_pmap.h"
static uint32_t table[PORT_PMAP_ENTRIES];
int main(void) {
    port_pmap_task(table,0x20000u,0x20800u,0x30000u,0x31000u,
                   0x50000u,0x51000u);
    assert(table[0x20] == (0x20000u | 5u)); /* RX user text, not writable */
    assert(table[0x30] == (0x30000u | 7u)); /* RW user stack */
    assert(table[0x50] == (0x50000u | 5u)); /* RO user clock */
    assert(table[0x31] == (0x31000u | 3u)); /* other task stack hidden */
    assert(table[0x10] == (0x10000u | 3u)); /* kernel remains private */
    assert(table[0] == 3u);
    port_pmap_user_range(table,0x3ff000u,0x401000u,1);
    assert(table[0x3ff] == (0x3ff000u | 3u)); /* invalid range rejects */
    port_pmap_user_range(table,0x38004u,0x38008u,1);
    assert(table[0x38] == (0x38000u | 7u)); /* align to page */
    port_pmap_guard_page(table,0x37000u);
    assert(table[0x37]==0); /* supervisor #PF guard */
    port_pmap_guard_page(table,0x37001u);
    assert(table[0x37]==0); /* unaligned guard rejected */
    port_pmap_guard_page(table,0x400000u);
    assert(table[0x3ff] == (0x3ff000u | 3u)); /* outside first PDE */
    puts("PASS: task permissions, range guard, kernel-stack guard PTE");
    return 0;
}
