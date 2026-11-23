#include "vms_event.h"
void port_event_init(struct port_event_cluster *c) {
    if (!c) return;
    c->flags=c->sets=c->clears=0;
}
void port_event_set(struct port_event_cluster *c,uint32_t mask) {
    if (!c) return;
    c->flags|=mask;
    ++c->sets;
}
void port_event_clear(struct port_event_cluster *c,uint32_t mask) {
    if (!c) return;
    c->flags&=~mask;
    ++c->clears;
}
int port_event_satisfied(const struct port_event_cluster *c,
                         uint32_t mask,int all) {
    if (!c || !mask) return 0;
    return all ? (c->flags&mask)==mask : (c->flags&mask)!=0;
}
