/* Native IA-32 partial translation of RTL/SRC/FORIOBEG.B32, pinned VMS 3.7.
 * Original lines 1..824 with public entry points:
 * 305: FOR$$IO_BEG
 * Native unit state and backend dispatch are bounded, tested operations.
 * RMS, full VAX condition handling, recursion and runtime ASTs remain unresolved.
 * No original proprietary DEC source has been included.
 */
#include "../vms_for_m112.h"
int port112_foriobeg(struct port112_unit *u,const struct port112_backend *b,
                        uint32_t statement){
    /* Original receives FLAGS_ARG in R0 and UNIT on AP; native ABI supplies
     * both explicitly. Real RMS open/dispatch remains a backend dependency. */
    return port112_io_begin(u,b,statement);
}
