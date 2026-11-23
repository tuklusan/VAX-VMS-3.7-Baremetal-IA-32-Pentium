#include "vms_ne2k.h"
static uint8_t rx8(void *ctx,uint16_t port) {
    (void)ctx; uint8_t value;
    __asm__ volatile("inb %w1,%b0":"=a"(value):"Nd"(port));
    return value;
}
static uint16_t rx16(void *ctx,uint16_t port) {
    (void)ctx; uint16_t value;
    __asm__ volatile("inw %w1,%w0":"=a"(value):"Nd"(port));
    return value;
}
static void tx8(void *ctx,uint16_t port,uint8_t value) {
    (void)ctx;
    __asm__ volatile("outb %b0,%w1"::"a"(value),"Nd"(port));
}
static void tx16(void *ctx,uint16_t port,uint16_t value) {
    (void)ctx;
    __asm__ volatile("outw %w0,%w1"::"a"(value),"Nd"(port));
}
static struct port_ne2k_bus const bus={rx8,rx16,tx8,0,tx16};
int port_ne2k_x86_probe(struct port_ne2k *nic) {
    return port_ne2k_probe(nic,&bus,0x300);
}
int port_ne2k_x86_pending(struct port_ne2k *nic) {
    return port_ne2k_pending(nic,&bus);
}

int port_ne2k_x86_send(struct port_ne2k *nic,const uint8_t *frame,unsigned len) {
    return port_ne2k_send(nic,&bus,frame,len);
}
int port_ne2k_x86_recv(struct port_ne2k *nic,uint8_t *frame,unsigned capacity) {
    return port_ne2k_recv(nic,&bus,frame,capacity);
}

int port_ne2k_x86_set_station(struct port_ne2k *nic,const uint8_t mac[6]) {
    return port_ne2k_set_station(nic,&bus,mac);
}
