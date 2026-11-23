#ifndef VMS_RTL_M109_X87_H
#define VMS_RTL_M109_X87_H
/* Freestanding IA-32 hardware FPU operations. No libm, Linux, or VAX ISA. */
long double port109_x87_sqrt(long double x);
long double port109_x87_atan2(long double y,long double x);
long double port109_x87_ln(long double x);
long double port109_x87_log10(long double x);
long double port109_x87_log2(long double x);
long double port109_x87_exp(long double x);
long double port109_x87_scale(long double x,int e);
long double port109_x87_extract(long double x,int *e);

/* M110 uses the native x87 trig instructions; the VAX polynomial tables
 * and DEC error/PSL signaling remain independently unresolved. */
long double port109_x87_sin(long double x);
long double port109_x87_cos(long double x);
#endif
