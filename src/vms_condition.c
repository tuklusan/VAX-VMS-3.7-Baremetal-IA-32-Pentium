/* Original condition-dispatch design for a native IA-32 porting environment.
 * Distinct from the original VAX signal arrays, CHF handlers, frame walking,
 * and VAX unwind instructions, which are NOT implemented yet.
 */
#include "vms_condition.h"
void port_condition_init(struct port_condition_process *p) {
    if (!p) return;
    p->handlers=0;
    p->signals=0; p->handled=0; p->fatal=0;
    p->unwind_requests=0; p->signal_depth=0;
    p->last.type=PORT_COND_UNKNOWN;
    p->last.vector=p->last.error=p->last.eip=p->last.cs=0;
    p->last.eflags=p->last.fault_va=0;
    p->last.mode=p->last.continuable=0;
}
int port_condition_push(struct port_condition_process *p,
                        struct port_condition_handler *h, unsigned mode,
                        port_condition_fn callback, void *context) {
    if (!p || !h || !callback || mode>3 || h->attached) return -1;
    h->callback=callback;
    h->context=context;
    h->mode=(uint8_t)mode;
    h->previous=p->handlers;
    h->attached=1;
    p->handlers=h;
    return 0;
}
int port_condition_pop(struct port_condition_process *p,
                       struct port_condition_handler *h) {
    if (!p || !h || p->handlers!=h || !h->attached) return -1;
    p->handlers=h->previous;
    h->previous=0;
    h->attached=0;
    return 0;
}
enum port_condition_type port_condition_from_x86(uint32_t vector) {
    switch (vector) {
        case 0: return PORT_COND_DIVIDE;
        case 1: return PORT_COND_DEBUG;
        case 3: return PORT_COND_BREAK;
        case 4: case 5: case 16: case 19: return PORT_COND_ARITHMETIC;
        case 6: return PORT_COND_BAD_OPCODE;
        case 8: case 12: return PORT_COND_STACK;
        case 10: case 11: case 13: return PORT_COND_PROTECTION;
        case 14: return PORT_COND_PAGE;
        case 17: return PORT_COND_ALIGNMENT;
        case 18: return PORT_COND_MACHINE;
        default: return PORT_COND_UNKNOWN;
    }
}
const char *port_condition_name(enum port_condition_type type) {
    switch (type) {
        case PORT_COND_DIVIDE: return "arithmetic divide";
        case PORT_COND_DEBUG: return "debug exception";
        case PORT_COND_BREAK: return "breakpoint";
        case PORT_COND_BAD_OPCODE: return "reserved instruction";
        case PORT_COND_STACK: return "stack exception";
        case PORT_COND_ACCESS: return "access violation";
        case PORT_COND_ARITHMETIC: return "arithmetic exception";
        case PORT_COND_PROTECTION: return "general protection";
        case PORT_COND_PAGE: return "page fault";
        case PORT_COND_ALIGNMENT: return "alignment fault";
        case PORT_COND_MACHINE: return "machine check";
        case PORT_COND_AST_STACK: return "AST stack fault";
        default: return "unclassified exception";
    }
}
enum port_condition_action port_condition_raise(struct port_condition_process *p,
                                                 const struct port_condition *signal) {
    if (!p || !signal || signal->mode>3) return PORT_COND_STOP;
    p->last=*signal;
    ++p->signals;
    /* Bounded nesting protects the exception dispatcher from recursion loops. */
    if (p->signal_depth>=4) { ++p->fatal; return PORT_COND_STOP; }
    ++p->signal_depth;
    struct port_condition_handler *h=p->handlers;
    enum port_condition_action answer=PORT_COND_STOP;
    while (h) {
        /* A handler registered for one access mode is not magically valid
         * on the others. Actual VMS stack searches come later. */
        if (h->mode == signal->mode) {
            answer=h->callback(signal,h->context);
            if (answer==PORT_COND_RESIGNAL) { h=h->previous; continue; }
            if (answer==PORT_COND_CONTINUE && !signal->continuable)
                answer=PORT_COND_STOP;
            if (answer!=PORT_COND_CONTINUE && answer!=PORT_COND_UNWIND)
                answer=PORT_COND_STOP;
            break;
        }
        h=h->previous;
    }
    if (answer==PORT_COND_RESIGNAL) answer=PORT_COND_STOP;
    --p->signal_depth;
    if (answer==PORT_COND_CONTINUE) ++p->handled;
    else if (answer==PORT_COND_UNWIND) ++p->unwind_requests;
    else ++p->fatal;
    return answer;
}
