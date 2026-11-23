#ifndef PORTLAB_VMS_EVENT_H
#define PORTLAB_VMS_EVENT_H
#include <stdint.h>
/* VMS-inspired event flag cluster. This is an original, simplified 32-bit
 * model; these are not VMS's exact event flag numbers or binary ABI. */
struct port_event_cluster { uint32_t flags; uint32_t sets; uint32_t clears; };
void port_event_init(struct port_event_cluster *c);
void port_event_set(struct port_event_cluster *c, uint32_t mask);
void port_event_clear(struct port_event_cluster *c, uint32_t mask);
int port_event_satisfied(const struct port_event_cluster *c,
                         uint32_t mask, int all);
#endif
