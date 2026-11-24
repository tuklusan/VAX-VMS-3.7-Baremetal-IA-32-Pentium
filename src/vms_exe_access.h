#ifndef VMS_EXE_ACCESS_H
#define VMS_EXE_ACCESS_H
#include <stdint.h>
#include "vms_userptr.h"
/* Caller-mode page checks use the running process's native page tables. */
struct port_exe_access {
    void *context;
    int (*read)(void *,uint32_t,void *,uint32_t);
    int (*write)(void *,uint32_t,const void *,uint32_t);
    int (*can_read)(void *,uint32_t,uint32_t);
    int (*can_write)(void *,uint32_t,uint32_t);
    int (*lock_io)(void *,uint32_t,uint32_t,int,
                   struct port_user_page_locks *);
};
/* Bind executive caller-mode copies to a process page map. The physical
 * resolver returns kernel aliases; it must not expose user virtual pointers. */
struct port_exe_user_access {
    const struct port_user_map *map;
    port_phys_page_resolver physical;
    void *physical_context;
    port_user_page_pin pin;
    port_user_page_unpin unpin;
    void *lock_context;
};
void port_exe_user_access_init(struct port_exe_access *,
                              struct port_exe_user_access *,
                              const struct port_user_map *,
                              port_phys_page_resolver,void *);
void port_exe_user_access_init_pinned(struct port_exe_access *,
                              struct port_exe_user_access *,
                              const struct port_user_map *,
                              port_phys_page_resolver,void *,
                              port_user_page_pin,port_user_page_unpin,void *);
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
static inline int port_exe_lock_io(const struct port_exe_access *a,
                                  uint32_t addr,uint32_t bytes,int writing,
                                  struct port_user_page_locks *locks) {
    return a&&a->lock_io&&port_exe_addr_ok(addr,bytes)&&
           a->lock_io(a->context,addr,bytes,writing,locks)>0;
}
#endif
