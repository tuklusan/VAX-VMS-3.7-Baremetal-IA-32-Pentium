#ifndef VMS_F11_ACP87_H
#define VMS_F11_ACP87_H
#include <stddef.h>
#include <stdint.h>
#include "vms_f11_acp86.h"
#include "vms_v37_dyn_tags.h"

enum {
    PORT_F11_87_NORMAL=1u, PORT_F11_87_BADPARAM=20u,
    PORT_F11_87_WRONGACP=796u, PORT_F11_87_BADCHKSUM=2056u,
    PORT_F11_87_BADFILEHDR=2064u, PORT_F11_87_FILENUMCHK=2224u,
    PORT_F11_87_FILESTRUCT=2240u, PORT_F11_87_NOSUCHFILE=2320u,
    PORT_F11_87_INCOMPLETE=0xfffffffcu
};
enum { PORT_F11_87_FCBTYPE=PORT_V37_DYN_FCB,
       PORT_F11_87_WCBTYPE=PORT_V37_DYN_WCB,
       PORT_F11_87_MIN_WINDOW=1,
       PORT_F11_87_MAX_WINDOW=80, PORT_F11_87_FIBLEN=80,
       PORT_F11_87_FIB_ACCDATA=10, PORT_F11_87_IO_DEACCESS=52,
       PORT_F11_87_IO_ACPCONTROL=56, PORT_F11_87_IO_WRITEPBLK=11,
       PORT_F11_87_IO_READPBLK=12, PORT_F11_87_IRP_FUNC=2,
       PORT_F11_87_IRP_COMPLX=8, PORT_F11_87_IRP_VIRTUAL=16 };

/* ACP file control block and its bidirectional VCB list. */
struct port_f11_87_packet;
struct port_f11_87_window;
struct port_f11_87_fcb {
    struct port_f11_87_fcb *next,*prev;
    struct port_f11_87_fcb *extension;
    struct port_f11_87_window *windows;
    struct port_f11_87_window *window_tail;
    struct port_f11_86_fid fid;
    uint32_t header_lbn, first_lbn, file_blocks, end_block, start_vbn;
    uint32_t owner;
    uint16_t protection, versions, segment;
    uint16_t accesses, writers, no_writers, no_truncate;
    uint16_t dirseq;
    uint8_t dirindex[16];
    uint8_t type, directory, exclusive, spool, contiguous;
};
struct port_f11_87_volume {
    struct port_f11_87_fcb *head,*tail;
    int32_t directory_lru;
    uint16_t cluster, current_rvn, transactions;
    uint8_t index_bitmap_blocks, reserved_files, extended_fid, default_window;
    uint8_t write_index,write_bitmap;
    void *rvtable;
};
struct port_f11_87_window {
    struct port_f11_87_window *next_segment,*next_fcb;
    struct port_f11_87_fcb *fcb;
    void *ucb,*rvtable;
    uint32_t pid;
    uint16_t access, pointer_count;
    uint8_t read, write, write_access, read_check, write_check;
    uint8_t no_read, no_write, no_truncate;
    uint8_t expire, allocation_owned;
    void *allocation_base;
    uint32_t allocation_bytes;
};
struct port_f11_87_ops {
    void *context;
    void *(*alloc)(void *,uint32_t,uint32_t);
    uint32_t (*dealloc)(void *,void *,uint32_t,uint32_t);
    uint32_t (*kernel_mount)(void *,uint32_t *,uint32_t);
    uint32_t (*turn_window)(void *,struct port_f11_87_window *,const uint8_t[512],
                            uint32_t,uint32_t,struct port_f11_87_window **);
    uint32_t (*map_vbn)(void *,uint32_t,uint32_t *);
    uint32_t (*read_lbn)(void *,uint32_t,uint8_t[512]);
    uint32_t (*switch_volume)(void *,uint16_t);
    void (*invalidate)(void *,uint32_t);
    uint32_t (*post_io)(void *,struct port_f11_87_packet *);
    uint32_t (*check_dismount)(void *);
};
struct port_f11_87_packet {
    struct port_f11_87_packet *next;
    uint32_t status, value, saved_page, user_page;
    uint16_t io_func, flags, byte_count, original_bytes;
    struct port_f11_87_volume *volume;
};
struct port_f11_87_context {
    struct port_f11_87_volume *volume;
    struct port_f11_87_fcb *primary;
    struct port_f11_87_window *window;
    struct port_f11_87_ops ops;
    void *ucb;
    uint32_t open_count,total_opens;
    uint8_t cleanup_deaccess,cleanup_directory,cleanup_spool;
    uint16_t status;
};
struct port_f11_87_deaccess_request {
    uint8_t extend,truncate,attributes,reserved;
};

uint32_t port_f11a_mount(struct port_f11_87_context *,uint8_t,uint32_t *);
uint32_t port_f11b_mount(struct port_f11_87_context *,uint8_t,uint32_t *);
struct port_f11_87_fcb *port_f11a_search_fcb(struct port_f11_87_volume *,
                                             struct port_f11_86_fid);
struct port_f11_87_fcb *port_f11b_search_fcb(struct port_f11_87_volume *,
                                             struct port_f11_86_fid);
size_t port_f11a_make_string(const uint8_t[10],uint8_t[24]);
size_t port_f11b_make_string(const uint8_t[10],uint8_t[24]);
uint32_t port_f11a_check_header(uint8_t[512],struct port_f11_86_fid,uint32_t *);
uint32_t port_f11b_check_header2(uint8_t[512],struct port_f11_86_fid,int,uint32_t *);
uint32_t port_f11a_init_fcb(struct port_f11_87_fcb *,const uint8_t[512],uint32_t);
uint32_t port_f11b_init_fcb2(struct port_f11_87_fcb *,const uint8_t[512],uint32_t,
                              struct port_f11_87_volume *);
uint32_t port_f11b_fill_fcb(struct port_f11_87_fcb *,const uint8_t[512],uint32_t,
                             struct port_f11_87_volume *);
struct port_f11_87_fcb *port_f11a_create_fcb(struct port_f11_87_context *,
                                              const uint8_t[512],uint32_t);
struct port_f11_87_fcb *port_f11b_create_fcb(struct port_f11_87_context *,
                                              const uint8_t[512],uint32_t);
uint32_t port_f11a_update_fcb(struct port_f11_87_context *,const uint8_t[512],uint32_t);
uint32_t port_f11b_update_fcb(struct port_f11_87_context *,const uint8_t[512],uint32_t);
uint32_t port_f11b_make_pointer(uint8_t[512],uint16_t *,uint32_t,uint32_t,uint16_t);
uint32_t port_f11a_read_header(struct port_f11_87_context *,const struct port_f11_86_fid *,
                                struct port_f11_87_fcb *,uint8_t[512],uint32_t *);
uint32_t port_f11b_read_header(struct port_f11_87_context *,const struct port_f11_86_fid *,
                                struct port_f11_87_fcb *,uint8_t[512],uint32_t *);
struct port_f11_87_window *port_f11a_create_window(struct port_f11_87_context *,
     uint16_t,int32_t,const uint8_t[512],uint32_t,struct port_f11_87_fcb *);
struct port_f11_87_window *port_f11b_create_window(struct port_f11_87_context *,
     uint16_t,int32_t,const uint8_t[512],uint32_t,struct port_f11_87_fcb *);
struct port_f11_87_fib_result {
    uint8_t bytes[PORT_F11_87_FIBLEN];
    uint16_t length;
};
uint32_t port_f11a_get_fib(struct port_f11_87_context *,const uint8_t *,size_t,
                           uint8_t,struct port_f11_87_fib_result *);
uint32_t port_f11b_get_fib(struct port_f11_87_context *,const uint8_t *,size_t,
                           uint8_t,struct port_f11_87_fib_result *);
uint32_t port_f11a_make_access(struct port_f11_87_context *,struct port_f11_87_fcb *,
                                struct port_f11_87_window *,struct port_f11_87_window **);
uint32_t port_f11b_make_access(struct port_f11_87_context *,struct port_f11_87_fcb *,
                                struct port_f11_87_window *,struct port_f11_87_window **);
/* DEACCS.B32's read-only, non-expiring, no-attribute close path. Other
 * source branches return INCOMPLETE until header mutation and cleanup
 * services are integrated. */
uint32_t port_f11b_deaccess(struct port_f11_87_context *,
                            const struct port_f11_87_deaccess_request *);
uint32_t port_f11a_io_done(struct port_f11_87_context *,struct port_f11_87_packet *,
                            uint32_t,uint32_t,struct port_f11_86_abd *,const uint8_t *,size_t);
uint32_t port_f11b_io_done(struct port_f11_87_context *,struct port_f11_87_packet *,
                            uint32_t,uint32_t,struct port_f11_86_abd *,const uint8_t *,size_t);
#endif
