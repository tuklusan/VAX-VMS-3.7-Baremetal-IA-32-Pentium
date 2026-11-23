#ifndef VMS_EXE_ACCESS_H
#define VMS_EXE_ACCESS_H
#include <stdint.h>
/* Caller-mode page checks use the running process's native page tables. */
struct port_exe_access {
    void *context;
    int (*read)(void *,uint32_t,void *,uint32_t);
    int (*write)(void *,uint32_t,const void *,uint32_t);
    int (*can_read)(void *,uint32_t,uint32_t);
    int (*can_write)(void *,uint32_t,uint32_t);
};
static inline int port_exe_addr_ok(uint32_t addr,uint32_t bytes) {
    return !bytes || addr<=UINT32_MAX-(bytes-1u);
}
static inline int port_exe_read(const struct port_exe_access *a,uint32_t addr,
                                void *dst,uint32_t size) {
    return a && a->read && port_exe_addr_ok(addr,size) &&
           a->read(a->context,addr,dst,size);
}
static inline int port_exe_write(const struct port_exe_access *a,uint32_t addr,
                                 const void *src,uint32_t size) {
    return a && a->write && port_exe_addr_ok(addr,size) &&
           a->write(a->context,addr,src,size);
}
static inline int port_exe_probe_read(const struct port_exe_access *a,
                                     uint32_t addr,uint32_t size) {
    return a && a->can_read && port_exe_addr_ok(addr,size) &&
           a->can_read(a->context,addr,size);
}
static inline int port_exe_probe_write(const struct port_exe_access *a,
                                      uint32_t addr,uint32_t size) {
    return a && a->can_write && port_exe_addr_ok(addr,size) &&
           a->can_write(a->context,addr,size);
}
#endif
