#ifndef VMS_EXE_NULLPROC_H
#define VMS_EXE_NULLPROC_H
/* SYS/SRC/NULLPROC.MAR: never returns; idle until another process runs. */
void port_exe_nullproc(void) __attribute__((noreturn));
#endif
