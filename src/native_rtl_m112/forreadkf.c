/* Native IA-32 source-specific VMS 3.7 FORTRAN dispatch translation.
 * Original RTL/SRC/FORREADKF.MAR lines 1..181 (entry 175: FOR$READ_KF).
 * Implements source MOVZB[L/W] statement type and JMP FOR$$IO_BEG+2.
 * No DEC source is copied. User VAX call frame, RMS and guest binding unresolved.
 */
#include "../vms_for_m112.h"
int port112_forreadkf(struct port112_unit *unit,const struct port112_backend *backend){
    return port112_io_begin(unit,backend,14u);
}
