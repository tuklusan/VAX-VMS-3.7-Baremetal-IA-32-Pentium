#ifndef VMS_EXE_GETSYI_H
#define VMS_EXE_GETSYI_H
#include <stdint.h>
#include "vms_exe_gettim.h"

/* SYS/SRC/SYSGETSYI.MAR, V03-006 (1982): only three data items exist. */
enum {
    PORT_SYI_VERSION=0x0100u, PORT_SYI_CPUTYPE=0x0200u,
    PORT_SYI_SID=0x0201u
};

/* Operations run with the calling process access checks and event flags.
 * The read and write calls use native 32-bit virtual addresses. */
typedef int (*port_syi_read)(void *,uint32_t,void *,uint32_t);
typedef int (*port_syi_write)(void *,uint32_t,const void *,uint32_t);
typedef int (*port_syi_probe)(void *,uint32_t,uint32_t);
typedef uint32_t (*port_syi_clear)(void *,uint32_t);
typedef void (*port_syi_post)(void *,uint32_t,uint32_t);
typedef void (*port_syi_ast)(void *,uint32_t,uint32_t,uint32_t);

struct port_syi_process {
    uint32_t pid;
    uint16_t astcnt;
    uint8_t previous_mode;
    uint8_t _reserved;
};
struct port_syi_system {
    uint8_t version[8];
    uint8_t cpu_type;
    uint8_t _pad[3];
    uint32_t sid;
};
struct port_syi_ops {
    port_syi_read read;
    port_syi_write write;
    port_syi_probe probe_write;
    port_syi_clear clear_event;
    port_syi_post post_event;
    port_syi_ast declare_ast;
    void *context;
};
struct port_syi_request {
    uint32_t efn;
    uint32_t reserved1;
    uint32_t reserved2;
    uint32_t item_list;
    uint32_t iosb;
    uint32_t ast_address;
    uint32_t ast_parameter;
};

uint32_t port_exe_getsyi(const struct port_syi_process *process,
                         const struct port_syi_system *system,
                         const struct port_syi_ops *ops,
                         const struct port_syi_request *request);

/* Read/write under the live IA-32 process page tables. */
struct port_syi_user {
    const struct port_user_map *map;
    port_phys_page_resolver physical;
    void *context;
};
int port_syi_user_read(void *context,uint32_t address,void *target,uint32_t bytes);
int port_syi_user_write(void *context,uint32_t address,const void *source,uint32_t bytes);
int port_syi_user_probe(void *context,uint32_t address,uint32_t bytes);
#endif
