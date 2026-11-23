#ifndef PORT_VMS_DCL_EXIT_H
#define PORT_VMS_DCL_EXIT_H
#include <stdint.h>

enum {PORT_DCL_COMMAND=1u, PORT_DCL_INHIB_MSG=0x10000000u,
      PORT_DCL_WAIT_EFN=31u, PORT_DCL_EXIT_RETURN=1,
      PORT_DCL_EXIT_PARSE_ERROR=0, PORT_DCL_EXIT_LOGOUT=2};
struct port_dcl_process {
    uint32_t last_status;
    uint32_t command_flags;
    uint16_t indirect_depth;
    uint8_t interactive;
    uint8_t exiting;
};
struct port_dcl_ops {
    /* DCL$BINEXPR returns an odd completion code on success. */
    uint32_t (*binary_expression)(void *ctx,const char *expr,uint32_t *value);
    void (*unstack)(void *ctx);
    void (*rundown)(void *ctx);
    void (*abort_process)(void *ctx);
    int (*cancel_timers)(void *ctx);
    int (*binary_delta)(void *ctx,const char *spec,uint64_t *ticks100ns);
    int (*set_timer)(void *ctx,unsigned efn,uint64_t ticks100ns);
    int (*wait_flag)(void *ctx,unsigned efn);
};
/* EXIT optionally changes $STATUS; EXIT inside a command file un-stacks
 * one depth. At depth 0 the interactive process runs exit handlers,
 * while batch processing aborts its session. */
int port_dcl_exit(struct port_dcl_process *p,const char *expression,
                  const struct port_dcl_ops *ops,void *ctx,uint32_t *status);
/* WAIT delegates native $BINTIM/$SETIMR/$WAITFR to the caller's services. */
int port_dcl_wait(const char *spec,const struct port_dcl_ops *ops,void *ctx);
/* Convert the WAIT operand with zero days (0 HH:MM:SS.cc). */
int port_dcl_wait_delta(const char *spec,uint64_t *ticks100ns);
#endif
