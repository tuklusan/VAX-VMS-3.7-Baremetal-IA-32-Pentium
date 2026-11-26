#ifndef VMS_BINTIM_H
#define VMS_BINTIM_H
#include <stddef.h>
#include <stdint.h>

enum { PORT_BINTIM_NORMAL=1u, PORT_BINTIM_IVTIME=2u };
enum { PORT_NUMTIM_NORMAL=1u, PORT_NUMTIM_IVTIME=2u,
       PORT_NUMTIM_ACCVIO=4u };

/* Native SYS$BINTIM/EXE$BINTIM syntax converter. Absolute inputs use the
 * current binary time for omitted fields, as EXE$NUMTIM does; delta results
 * are returned as negative 100-nanosecond counts. */
uint32_t port_vms_bintim(const char *,size_t,uint64_t,uint64_t *);

/* Native EXE$NUMTIM conversion core. `binary_time == NULL` selects the
 * supplied current-time snapshot; negative values produce relative-day
 * fields (year/month zero), and the output is committed only on success. */
uint32_t port_vms_numtim(uint16_t numeric_time[7],
                         const uint64_t *binary_time,
                         uint64_t current_time);

#endif
