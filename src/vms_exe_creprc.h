#ifndef VMS_EXE_CREPRC_H
#define VMS_EXE_CREPRC_H
#include <stdint.h>

enum {
    PORT_V37_PRC_STATUS_FLAGS=0x1ffu,
    PORT_V37_PRC_PRIV_PSWAPM=12u,
    PORT_V37_PRC_PRIV_NOACNT=9u,
    PORT_V37_PRC_PRIV_DETACH=5u,
    PORT_V37_PRC_PRIV_SETPRI=13u,
    PORT_V37_PRC_PRIV_SETPRV=14u
};

struct port_exe_creprc_caller {
    uint32_t pid,uic;
    uint8_t base_priority; /* PCB$B_PRIB internal form: 31 - external */
    uint64_t privilege_mask; /* Current process PHD$Q_PRIVMSK */
};
struct port_exe_creprc_request {
    uint32_t pid_output_address;
    uint32_t uic,status_flags,base_priority;
    uint32_t privilege_address;
    uint64_t requested_privileges;
    uint8_t pid_output_writable;
    uint8_t privilege_readable;
};
struct port_exe_creprc_result {
    uint32_t uic,owner_pid,status_flags;
    uint8_t internal_priority;
    uint64_t privilege_mask;
};

/* SYSCREPRC.MAR pre-allocation policy: validate output/privilege operands,
 * enforce DETACH, mask status flags, inherit subprocess identity, minimize
 * requested privileges and priority, and leave output untouched on error.
 * Later PCB/JIB/PQB allocation, status-bit translation into PCB state, quotas, duplicate
 * names, image activation and PID publication are separate stages. */
uint32_t port_exe_creprc_preflight(const struct port_exe_creprc_caller *,
    const struct port_exe_creprc_request *,struct port_exe_creprc_result *);
#endif
