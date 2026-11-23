#include "vms_ramcheck.h"
/* Check A20, A21, and the last pool page using disjoint signature bytes.
 * Keep the sample points away from the boot sector, BDA and kernel image.
 * All original bytes are restored, even when an alias is detected. */
int port_ram_probe_3mib(const struct port_ram_probe_bus *bus) {
    if(!bus||!bus->read8||!bus->write8)return 0;
    static const uint32_t address[4]={0x00000500u,0x00100500u,
                                      0x00200500u,0x002ff500u};
    static const uint8_t probe[4]={0x13u,0x57u,0x9bu,0xdfu};
    uint8_t saved[4];
    for(unsigned i=0;i<4;i++)saved[i]=bus->read8(bus->context,address[i]);
    for(unsigned i=0;i<4;i++)bus->write8(bus->context,address[i],probe[i]);
    int ok=1;
    for(unsigned i=0;i<4;i++)
        if(bus->read8(bus->context,address[i])!=probe[i])ok=0;
    /* Restore in reverse order, consistent even with mirrored addresses. */
    for(unsigned i=4;i>0;i--)
        bus->write8(bus->context,address[i-1],saved[i-1]);
    return ok;
}
