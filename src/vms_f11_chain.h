#ifndef VMS37_NATIVE_F11_CHAIN_H
#define VMS37_NATIVE_F11_CHAIN_H
#include "vms_f11_bootpath.h"
#include "vms_boot_files11.h"
/* FIL$RDCHKFILHDR extension FID chain and normalized retrieval list.
 * Bounded supervisor-only Files-11 read-only stage, not a mounted ACP. */
enum {PORT_F11_CHAIN_OK=1u,PORT_F11_CHAIN_BAD=2u,
      PORT_F11_CHAIN_IO=4u,PORT_F11_CHAIN_HEADER=6u,
      PORT_F11_CHAIN_MAP=8u,PORT_F11_CHAIN_CYCLE=10u};
#define PORT_F11_CHAIN_MAX_EXTENTS 64u
#define PORT_F11_CHAIN_MAX_HEADERS 16u
struct port_f11_chain {
    struct port_dq_dir_fid first;
    struct port_f11_extent extents[PORT_F11_CHAIN_MAX_EXTENTS];
    uint32_t count,blocks,headers,first_lbn;
};
uint32_t port_f11_chain_collect(const struct port_f11_boot_volume *,
    const struct port_dq_dir_fid *,struct port_f11_chain *);
uint32_t port_f11_chain_lbn(const struct port_f11_chain *,uint32_t,
                           uint32_t *);
uint32_t port_f11_chain_read(const struct port_f11_boot_volume *,
    const struct port_f11_chain *,uint32_t,uint32_t,uint8_t *,size_t,
    uint32_t *);
#endif
