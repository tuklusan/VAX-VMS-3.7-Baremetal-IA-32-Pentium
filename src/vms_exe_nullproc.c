#include "vms_exe_nullproc.h"
void port_exe_nullproc(void) {
    /* NULLPROC.MAR is an unconditional branch to itself (the source marks
     * this path temporary). Preserve that behavior; an HLT loop would change
     * its instruction stream and idle accounting semantics. */
    __asm__ __volatile__("1: jmp 1b" ::: "memory");
    __builtin_unreachable();
}
