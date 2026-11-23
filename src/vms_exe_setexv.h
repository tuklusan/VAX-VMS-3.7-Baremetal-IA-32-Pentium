#ifndef VMS_EXE_SETEXV_H
#define VMS_EXE_SETEXV_H
#include <stdint.h>
#include "vms_exe_gettim.h"
/* Native control-region exception vectors for SYS/SRC/SYSSETEXV.MAR. */
struct port_exe_exception_vectors {
    uint32_t normal[4][2];
    uint32_t last[4];
};
/* The caller controls copying a previous vector under its access mode.
 * A zero destination skips the copy. Mode 0,1,2,3 means K,E,S,U. */
typedef int (*port_exe_word_write)(void *context,uint32_t address,
                                    const uint32_t *value);
uint32_t port_exe_setexv(struct port_exe_exception_vectors *vectors,
                          uint32_t vector,uint32_t handler,
                          uint32_t requested_mode,uint32_t previous_mode,
                          uint32_t previous_address,
                          port_exe_word_write write,void *context);
struct port_exe_user_vector {
    const struct port_user_map *map;
    port_phys_page_resolver physical;
    void *context;
};
uint32_t port_exe_setexv_user(struct port_exe_exception_vectors *vectors,
                              uint32_t vector,uint32_t handler,
                              uint32_t requested_mode,uint32_t previous_mode,
                              uint32_t previous_address,
                              const struct port_exe_user_vector *user);
#endif
