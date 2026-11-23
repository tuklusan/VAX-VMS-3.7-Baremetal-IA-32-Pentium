#include "vms_exe_nullproc.h"
void port_exe_nullproc(void) {
    /* The source spins at the lowest priority. IA-32 waits for an IRQ. */
    for(;;) __asm__ __volatile__("sti; hlt" ::: "memory");
}
