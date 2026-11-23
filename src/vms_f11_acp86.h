#ifndef VMS_F11_ACP86_H
#define VMS_F11_ACP86_H
#include <stddef.h>
#include <stdint.h>

/* Files-11 structure-level 1 and 2 routines, per F11A/SRC and F11B/SRC. */
enum { PORT_F11_86_NORMAL=1u, PORT_F11_86_BADPARAM=20u,
       PORT_F11_86_BADFILEHDR=2064u, PORT_F11_86_IOERROR=8u };
struct port_f11_86_fid {
    uint16_t number, sequence, volume;
    uint8_t number_extension;
};
struct port_f11_86_abd {
    const uint8_t *name;
    uint16_t name_count;
    uint8_t *result;
    uint16_t result_count;
    uint16_t *result_length;
};
struct port_f11_86_map {
    const uint8_t *bytes;
    unsigned length;
    unsigned offset;
    uint32_t blocks, lbn;
};
uint32_t port_f11a_checksum(uint8_t header[512]);
uint32_t port_f11b_checksum(uint8_t header[512]);
uint32_t port_f11a_copy_name(const struct port_f11_86_abd *);
uint32_t port_f11b_copy_name(const struct port_f11_86_abd *);
uint32_t port_f11a_return_dir(const uint8_t *,uint16_t,const struct port_f11_86_abd *);
uint32_t port_f11b_return_dir(const uint8_t *,uint16_t,uint16_t,
                              const struct port_f11_86_abd *);
uint32_t port_f11b_get_map_pointer(struct port_f11_86_map *);
uint32_t port_f11b_file_size(const uint8_t header[512],uint32_t *blocks);
void port_f11b_apply_rvn(uint16_t *,uint16_t,int);
void port_f11b_default_rvn(uint16_t *,uint16_t,int);
uint32_t port_f11b_match_name(const uint8_t *,size_t,const uint8_t *,size_t);
void port_f11a_scan_badlog(void);

/* An ACP calls the resident executive for rights, mutex, and I/O work. */
typedef uint32_t (*port_f11_86_getaccess_fn)(void *,uint32_t,uint16_t);
uint32_t port_f11a_get_access(void *,port_f11_86_getaccess_fn,uint32_t,uint16_t);
uint32_t port_f11b_get_access(void *,port_f11_86_getaccess_fn,uint32_t,uint16_t);
struct port_f11_86_lockops {
    uint32_t (*lockw)(void *);
    uint32_t (*unlock)(void *);
    void (*set_ipl_zero)(void *);
    void *context;
};
uint32_t port_f11a_lock_iodb(const struct port_f11_86_lockops *);
uint32_t port_f11a_unlock_iodb(const struct port_f11_86_lockops *);
uint32_t port_f11b_lock_iodb(const struct port_f11_86_lockops *);
uint32_t port_f11b_unlock_iodb(const struct port_f11_86_lockops *);
struct port_f11_86_request {
    uint16_t bytes;
    uint16_t func;
    void *ucb;
    uint32_t lbn;
};
struct port_f11_86_iops {
    uint32_t (*logical_to_physical)(void *,void *,uint32_t,uint32_t *,uint16_t *);
    uint32_t (*insert_ioq)(void *,struct port_f11_86_request *);
    void *context;
};
uint32_t port_f11a_requeue(struct port_f11_86_request *,void *,uint32_t,
                          uint32_t,const struct port_f11_86_iops *);
uint32_t port_f11b_requeue(struct port_f11_86_request *,void *,uint32_t,
                          uint32_t,const struct port_f11_86_iops *);
struct port_f11_86_mapops {
    uint32_t (*map_vblock)(void *,void *,uint32_t,uint32_t,void **,
                           uint32_t *,uint32_t *);
    void *context;
};
int64_t port_f11a_map_window(void *,void *,uint32_t,uint32_t,uint32_t *,
                            const struct port_f11_86_mapops *);
int64_t port_f11b_map_window(void *,void *,uint32_t,uint32_t,uint32_t *,void **,
                            const struct port_f11_86_mapops *);
struct port_f11_86_file_control {void *extension_fcb;};
struct port_f11_86_headerops {
    uint32_t (*read_header)(void *,struct port_f11_86_fid,void *,uint8_t[512]);
    void *context;
};
uint32_t port_f11a_next_header(const uint8_t *,void *,const struct port_f11_86_fid *,
                              const uint16_t *,const struct port_f11_86_headerops *,
                              uint8_t[512],int *found);
uint32_t port_f11b_next_header(const uint8_t *,void *,const struct port_f11_86_fid *,
                              const uint16_t *,int,const struct port_f11_86_headerops *,
                              uint8_t[512],int *found);
/* System services SYS$NUMTIM and SYS$GETTIM supply the date to the ACP. */
struct port_f11_86_timeops {
    uint32_t (*now)(void *,uint64_t *);
    void *context;
};
uint32_t port_f11a_get_time(uint8_t[13],const uint64_t *,
                           const struct port_f11_86_timeops *);
uint32_t port_f11b_get_time(uint8_t[13],const uint64_t *,
                           const struct port_f11_86_timeops *);
#endif
