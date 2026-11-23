#ifndef PORTLAB_VMS_RAMCHECK_H
#define PORTLAB_VMS_RAMCHECK_H
#include <stdint.h>
/* Sampled alias probe for the fixed 1..3 MiB physical frame pool.
 * Call only with paging off and before the allocator owns the frames.
 * This is NOT a substitute for BIOS E820/physical RAM discovery. */
struct port_ram_probe_bus {
    uint8_t (*read8)(void *context,uint32_t address);
    void (*write8)(void *context,uint32_t address,uint8_t value);
    void *context;
};
int port_ram_probe_3mib(const struct port_ram_probe_bus *bus);
#endif
