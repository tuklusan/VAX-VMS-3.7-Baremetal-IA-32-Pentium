/* Native IA-32 partial translation of RTL/SRC/FORENDFIL.B32, pinned VMS 3.7.
 * Original lines 1..262 with public entry points:
 * 109: FOR$ENDFILE
 * Native unit state and backend dispatch are bounded, tested operations.
 * RMS, full VAX condition handling, recursion and runtime ASTs remain unresolved.
 * No original proprietary DEC source has been included.
 */
#include "../vms_for_m112.h"
int port112_forendfil(struct port112_unit *u,const struct port112_backend *b){return port112_endfile(u,b);}
