#ifndef VMS_SYS85_H
#define VMS_SYS85_H
#include <stdint.h>
#include <stddef.h>

/* Resident I/O, process RMS, disk-name and software fork routines. */
enum {
    PORT_SYS85_NORMAL=1u, PORT_SYS85_NOPRIV=36u,
    PORT_SYS85_WRITLCK=604u, PORT_SYS85_BADFILENAME=2072u,
    PORT_SYS85_ENDOFFILE=2160u, PORT_SYS85_MUSTCLOSEFL=2376u,
    PORT_SYS85_BADFUNC=20u
};
enum { PORT_SYS85_READLBLK=33u, PORT_SYS85_READVBLK=49u,
       PORT_SYS85_WRITEOF=40u, PORT_SYS85_WRITELBLK=32u,
       PORT_SYS85_WRITEVBLK=48u };

struct port_sys85_null_dpt {
    const char *name;
    uint16_t ucb_size;
    uint8_t device_functions[5];
    uint8_t count;
};
extern const struct port_sys85_null_dpt port_nl_driver_dpt;
typedef uint32_t (*port_sys85_finish_io_fn)(void *,uint32_t);
uint32_t port_nl_driver_io(uint16_t,void *,port_sys85_finish_io_fn);

struct port_sys85_rms_page { struct port_sys85_rms_page *next,*prev; };
struct port_sys85_rms {
    struct port_sys85_rms_page free_space, free_pages;
    uintptr_t segment_address;
    uint32_t segment_length;
    uint32_t ifab_table_link, irab_table_link;
    uint16_t active_ifabs;
    uint32_t *handle_table;
    size_t handle_table_count;
};
uint32_t port_rm_reset(struct port_sys85_rms *);

struct port_sys85_tape {
    uint32_t device_characteristics;
    uint32_t software_write_lock_mask;
    uint32_t owner_uic;
    uint16_t volume_protection;
};
typedef uint32_t (*port_sys85_check_write_fn)(void *,uint16_t,uint32_t);
typedef uint32_t (*port_sys85_abort_io_fn)(void *,uint32_t);
uint32_t port_mt_check_access(const struct port_sys85_tape *,void *,
    port_sys85_check_write_fn,port_sys85_abort_io_fn);

struct port_sys85_fnb { uint16_t filename[3],filetype,version; };
uint32_t port_fil_cvtfilnam(const uint8_t *,size_t,struct port_sys85_fnb *);

struct port_sys85_fork;
typedef void (*port_sys85_fork_routine)(struct port_sys85_fork *,void *);
struct port_sys85_fork {
    struct port_sys85_fork *next;
    port_sys85_fork_routine entry;
    void *arg;
    uint32_t saved_r3,saved_r4;
    uint8_t fork_ipl;
    uint8_t pending;
};
struct port_sys85_fork_system {
    struct port_sys85_fork *head[12],*tail[12];
    uint32_t pending_levels;
    void (*request_softint)(void *,uint8_t);
    void *irq_arg;
};
void port_exe_fork_init(struct port_sys85_fork_system *,
                        void (*)(void *,uint8_t),void *);
uint32_t port_exe_fork(struct port_sys85_fork_system *,struct port_sys85_fork *,
                       uint32_t,uint32_t);
uint32_t port_exe_iofork(struct port_sys85_fork_system *,struct port_sys85_fork *,
                         uint32_t,uint32_t,uint16_t *,uint16_t);
uint32_t port_exe_fork_dispatch(struct port_sys85_fork_system *,uint8_t);
#endif
