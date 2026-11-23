/* Native IA-32 source-specific VMS 3.7 FORTRAN dispatch translation.
 * Original RTL/SRC/FORREADSU.MAR lines 1..178 (entry 172: FOR$READ_SU).
 * Implements source MOVZB[L/W] statement type and JMP FOR$$IO_BEG+2.
 * No DEC source is copied. User VAX call frame, RMS and guest binding unresolved.
 */
#include "../vms_for_m112.h"
int port112_forreadsu(struct port112_unit *unit,const struct port112_backend *backend){
    return port112_io_begin(unit,backend,4u);
}
