#ifndef PORTLAB_VMS_CONDITION_H
#define PORTLAB_VMS_CONDITION_H
#include <stdint.h>

/* Condition handling bridge. Signal vectors are IA-32, NOT VMS status codes.
 * Current unwind is an explicit result, not real VMS stack unwinding. */
enum port_condition_type {
    PORT_COND_UNKNOWN=0, PORT_COND_DIVIDE, PORT_COND_DEBUG, PORT_COND_BREAK,
    PORT_COND_BAD_OPCODE, PORT_COND_STACK, PORT_COND_ACCESS,
    PORT_COND_ARITHMETIC, PORT_COND_PROTECTION, PORT_COND_PAGE,
    PORT_COND_ALIGNMENT, PORT_COND_MACHINE, PORT_COND_AST_STACK
};
enum port_condition_action {
    PORT_COND_RESIGNAL=0, PORT_COND_CONTINUE=1,
    PORT_COND_UNWIND=2, PORT_COND_STOP=3
};
struct port_condition {
    enum port_condition_type type;
    uint32_t vector, error, eip, cs, eflags, fault_va;
    uint8_t mode;
    uint8_t continuable;
};
typedef enum port_condition_action (*port_condition_fn)(
    const struct port_condition *signal, void *context);
struct port_condition_handler {
    struct port_condition_handler *previous;
    port_condition_fn callback;
    void *context;
    uint8_t mode;
    uint8_t attached;
};
struct port_condition_process {
    struct port_condition_handler *handlers;
    struct port_condition last;
    uint32_t signals, handled, fatal, unwind_requests;
    uint8_t signal_depth;
};
void port_condition_init(struct port_condition_process *p);
int port_condition_push(struct port_condition_process *p,
                        struct port_condition_handler *h, unsigned mode,
                        port_condition_fn callback, void *context);
int port_condition_pop(struct port_condition_process *p,
                       struct port_condition_handler *h);
enum port_condition_type port_condition_from_x86(uint32_t vector);
const char *port_condition_name(enum port_condition_type type);
enum port_condition_action port_condition_raise(struct port_condition_process *p,
                                                 const struct port_condition *signal);
#endif
