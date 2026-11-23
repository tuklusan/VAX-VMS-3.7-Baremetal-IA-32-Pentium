/* Native IA-32 partial translation of RTL/SRC/FORASSOC.B32, pinned VMS 3.7.
 * Original lines 1..169 with public entry points:
 * 101: FOR$$ASSOC
 * Native unit state and backend dispatch are bounded, tested operations.
 * RMS, full VAX condition handling, recursion and runtime ASTs remain unresolved.
 * No original proprietary DEC source has been included.
 */
#include "../vms_for_m112.h"
int port112_forassoc(struct port112_unit *u){return port112_associated(u);}
