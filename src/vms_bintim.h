#ifndef VMS_BINTIM_H
#define VMS_BINTIM_H
#include <stddef.h>
#include <stdint.h>

enum { PORT_BINTIM_NORMAL=1u, PORT_BINTIM_IVTIME=2u };

/* Native SYS$BINTIM/EXE$BINTIM syntax converter. Absolute inputs use the
 * current binary time for omitted fields, as EXE$NUMTIM does; delta results
 * are returned as negative 100-nanosecond counts. */
uint32_t port_vms_bintim(const char *,size_t,uint64_t,uint64_t *);

#endif
