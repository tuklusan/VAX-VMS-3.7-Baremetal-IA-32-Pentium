/* Native IA-32 partial translation of RTL/SRC/FORDELETE.B32, pinned VMS 3.7.
 * Original lines 1..462 with public entry points:
 * 110: FOR$DELETE
 * 243: FOR$DELETE_D
 * Native unit state and backend dispatch are bounded, tested operations.
 * RMS, full VAX condition handling, recursion and runtime ASTs remain unresolved.
 * No original proprietary DEC source has been included.
 */
#include "../vms_for_m112.h"
int port112_fordelete(struct port112_unit *u,const struct port112_backend *b){return port112_delete(u,b,0);}
int port112_fordelete_d(struct port112_unit *u,const struct port112_backend *b){return port112_delete(u,b,1);}
