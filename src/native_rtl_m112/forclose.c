/* Native IA-32 partial translation of RTL/SRC/FORCLOSE.B32, pinned VMS 3.7.
 * Original lines 1..403 with public entry points:
 * 177: FOR$CLOSE
 * Native unit state and backend dispatch are bounded, tested operations.
 * RMS, full VAX condition handling, recursion and runtime ASTs remain unresolved.
 * No original proprietary DEC source has been included.
 */
#include "../vms_for_m112.h"
int port112_forclose(struct port112_unit *u,const struct port112_backend *b){return port112_close(u,b);}
