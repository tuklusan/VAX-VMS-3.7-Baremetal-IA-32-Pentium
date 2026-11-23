#ifndef PORT_VMS_DCL_IF_H
#define PORT_VMS_DCL_IF_H
#include <stdint.h>

enum {PORT_DCL_IF_NOSTAT=4u, PORT_DCL_IF_RUN_THEN=2u};
struct port_dcl_if_ops {
    uint32_t normal_status;
    uint32_t invalid_then_status;
    uint32_t (*binary_expression)(void *ctx,const char *expr,uint32_t *value);
    void (*mark)(void *ctx);
    int (*command_start)(void *ctx,const char *then_command);
};
int port_dcl_if(const char *expression,const char *then_keyword,
                const char *then_command,uint32_t *flags,
                const struct port_dcl_if_ops *ops,void *ctx,
                uint32_t *completion);
#endif
