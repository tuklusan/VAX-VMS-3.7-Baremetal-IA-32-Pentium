#ifndef PORTLAB_VMS_PHYS_H
#define PORTLAB_VMS_PHYS_H
#include <stdint.h>
/* Reserve fixed, known-safe PC RAM above 1 MiB and below the 4 MiB
 * supervisor identity aperture. No BIOS memory discovery is claimed. */
enum { PORT_PHYS_BASE=0x00100000u, PORT_PHYS_FRAME_BYTES=4096,
       PORT_PHYS_FRAMES=512 };
struct port_phys_pool {
    uint32_t used[PORT_PHYS_FRAMES/32];
    uint16_t pins[PORT_PHYS_FRAMES];
    uint32_t available;
    uint32_t high_water;
};
void port_phys_init(struct port_phys_pool *p);
uint32_t port_phys_alloc(struct port_phys_pool *p); /* 0 on exhaustion */
int port_phys_free(struct port_phys_pool *p, uint32_t phys);
int port_phys_is_owned(const struct port_phys_pool *p, uint32_t phys);
/* IO-locked frames remain allocated until the matching completion unlock. */
int port_phys_pin(struct port_phys_pool *p, uint32_t phys);
int port_phys_unpin(struct port_phys_pool *p, uint32_t phys);
uint16_t port_phys_pin_count(const struct port_phys_pool *p,uint32_t phys);
uint32_t port_phys_available(const struct port_phys_pool *p);
#endif
