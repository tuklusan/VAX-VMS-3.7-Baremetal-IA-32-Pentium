#include "vms_phys.h"

static int frame_index(uint32_t address) {
    if (address<PORT_PHYS_BASE ||
        address>=PORT_PHYS_BASE+PORT_PHYS_FRAMES*PORT_PHYS_FRAME_BYTES ||
        (address&(PORT_PHYS_FRAME_BYTES-1u))) return -1;
    return (int)((address-PORT_PHYS_BASE)/PORT_PHYS_FRAME_BYTES);
}
void port_phys_init(struct port_phys_pool *p) {
    if (!p) return;
    for (unsigned i=0;i<PORT_PHYS_FRAMES/32;i++) p->used[i]=0;
    for (unsigned i=0;i<PORT_PHYS_FRAMES;i++) p->pins[i]=0;
    p->available=PORT_PHYS_FRAMES;
    p->high_water=0;
}
uint32_t port_phys_alloc(struct port_phys_pool *p) {
    if (!p || p->available==0) return 0;
    for (unsigned i=0;i<PORT_PHYS_FRAMES;i++) {
        uint32_t bit=1u<<(i&31u);
        if (!(p->used[i/32]&bit)) {
            p->used[i/32]|=bit;
            --p->available;
            uint32_t used=PORT_PHYS_FRAMES-p->available;
            if (used>p->high_water) p->high_water=used;
            return PORT_PHYS_BASE+i*PORT_PHYS_FRAME_BYTES;
        }
    }
    return 0;
}
int port_phys_free(struct port_phys_pool *p, uint32_t phys) {
    int idx=frame_index(phys);
    if (!p || idx<0) return -1;
    uint32_t bit=1u<<((unsigned)idx&31u);
    if (!(p->used[(unsigned)idx/32]&bit)) return -1; /* double free */
    if (p->pins[idx]) return -1; /* I/O still owns a stable frame */
    p->used[(unsigned)idx/32]&=~bit;
    ++p->available;
    return 0;
}
int port_phys_is_owned(const struct port_phys_pool *p, uint32_t phys) {
    int idx=frame_index(phys);
    return p && idx>=0 &&
           (p->used[(unsigned)idx/32]&(1u<<((unsigned)idx&31u)))!=0;
}
int port_phys_pin(struct port_phys_pool *p,uint32_t phys){
    const int idx=frame_index(phys);
    if(!p||idx<0||!port_phys_is_owned(p,phys)||p->pins[idx]==UINT16_MAX)
        return -1;
    ++p->pins[idx];
    return 0;
}
int port_phys_unpin(struct port_phys_pool *p,uint32_t phys){
    const int idx=frame_index(phys);
    if(!p||idx<0||!port_phys_is_owned(p,phys)||!p->pins[idx])return -1;
    --p->pins[idx];
    return 0;
}
uint16_t port_phys_pin_count(const struct port_phys_pool *p,uint32_t phys){
    const int idx=frame_index(phys);
    return p&&idx>=0&&port_phys_is_owned(p,phys)?p->pins[idx]:0u;
}
uint32_t port_phys_available(const struct port_phys_pool *p) {
    return p ? p->available:0;
}
