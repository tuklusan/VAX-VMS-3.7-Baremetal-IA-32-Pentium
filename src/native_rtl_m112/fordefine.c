/* Native IA-32 partial translation of RTL/SRC/FORDEFINE.B32, pinned VMS 3.7.
 * Original lines 1..328 with public entry points:
 * 135: FOR$DEF_FILE_W
 * 178: FOR$DEF_FILE
 * Native unit state and backend dispatch are bounded, tested operations.
 * RMS, full VAX condition handling, recursion and runtime ASTs remain unresolved.
 * No original proprietary DEC source has been included.
 */
#include "../vms_for_m112.h"
int port112_fordefine_w(struct port112_unit *u,uint32_t n,uint32_t words,void *assoc){
    return port112_define(u,n,words,assoc,2u);
}
int port112_fordefine_l(struct port112_unit *u,uint32_t n,uint32_t words,void *assoc){
    return port112_define(u,n,words,assoc,4u);
}
