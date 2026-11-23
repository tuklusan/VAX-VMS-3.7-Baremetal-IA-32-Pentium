#ifndef PORT_VMS_DCL_STATUS_H
#define PORT_VMS_DCL_STATUS_H
#include <stdint.h>
#include <stddef.h>

enum {PORT_DCL_STATUS_ONEXIT=1u, PORT_DCL_STATUS_YLEVEL=2u,
      PORT_DCL_STATUS_NOSTAT=4u, PORT_DCL_STATUS_ON_COMMAND=1u};
struct port_dcl_status_process {
    uint32_t last_status;
    uint8_t severity;
    char severity_text[2];
    char status_text[11];
    uint8_t on_level;
    uint8_t flags;
    const char *on_error;
};
struct port_dcl_status_work {
    uint8_t flags;
    char command[256];
    uint16_t char_index;
};
struct port_dcl_status_ops {
    void (*disable_control_ast)(void *ctx);
    void (*flush)(void *ctx);
    void (*on_reset)(void *ctx);
    void (*command_start)(void *ctx);
};
int port_dcl_set_status(struct port_dcl_status_process *process,
                        struct port_dcl_status_work *work,
                        uint32_t status,const struct port_dcl_status_ops *ops,
                        void *ctx);
#endif
