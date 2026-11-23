#include "vms_rtl_m109_x87.h"
/* Each operation starts and ends with a balanced x87 register stack.
 * We deliberately use the i386 FPU, not a VAX instruction emulator or
 * the host's libm. The translated DEC polynomial implementation remains
 * unported; callers report this distinction with PORT109_APPROX. */
long double port109_x87_sqrt(long double x){
    long double out;
    __asm__ __volatile__(
       "fldt %1\n\tfsqrt\n\tfstpt %0"
       : "=m"(out) : "m"(x) : "st");
    return out;
}
long double port109_x87_atan2(long double y,long double x){
    long double out;
    __asm__ __volatile__(
       "fldt %1\n\tfldt %2\n\tfpatan\n\tfstpt %0"
       : "=m"(out) : "m"(y),"m"(x) : "st","st(1)");
    return out;
}
long double port109_x87_ln(long double x){
    long double out;
    __asm__ __volatile__(
       "fldln2\n\tfldt %1\n\tfyl2x\n\tfstpt %0"
       : "=m"(out) : "m"(x) : "st","st(1)");
    return out;
}
long double port109_x87_log10(long double x){
    long double out;
    __asm__ __volatile__(
       "fldlg2\n\tfldt %1\n\tfyl2x\n\tfstpt %0"
       : "=m"(out) : "m"(x) : "st","st(1)");
    return out;
}
long double port109_x87_log2(long double x){
    long double out;
    __asm__ __volatile__(
       "fld1\n\tfldt %1\n\tfyl2x\n\tfstpt %0"
       : "=m"(out) : "m"(x) : "st","st(1)");
    return out;
}
long double port109_x87_exp(long double x){
    long double out;
    /* t = x log2(e). Split into nearest integer and fraction [-.5,.5].
     * 2^fraction = F2XM1(fraction) + 1, then scale by 2^integer. */
    __asm__ __volatile__(
       "fldt %1\n\tfldl2e\n\tfmulp %%st, %%st(1)\n\t"
       "fld %%st(0)\n\tfrndint\n\tfsubr %%st(0), %%st(1)\n\t"
       "fxch %%st(1)\n\tf2xm1\n\tfld1\n\tfaddp %%st, %%st(1)\n\t"
       "fscale\n\tfstp %%st(1)\n\tfstpt %0"
       : "=m"(out) : "m"(x) : "st","st(1)");
    return out;
}
long double port109_x87_scale(long double x,int e){
    long double out,exponent=(long double)e;
    __asm__ __volatile__(
       "fldt %1\n\tfldt %2\n\tfscale\n\tfstp %%st(1)\n\tfstpt %0"
       : "=m"(out) : "m"(exponent),"m"(x) : "st","st(1)");
    return out;
}
long double port109_x87_extract(long double x,int *e){
    long double normalized,exp;
    __asm__ __volatile__(
       "fldt %2\n\tfxtract\n\tfstpt %0\n\tfstpt %1"
       : "=m"(normalized),"=m"(exp) : "m"(x) : "st","st(1)");
    *e=(int)exp+1;
    return normalized*0.5L;
}
long double port109_x87_sin(long double x){
    long double out;
    __asm__ __volatile__("fldt %1\n\tfsin\n\tfstpt %0"
        : "=m"(out) : "m"(x) : "st");
    return out;
}
long double port109_x87_cos(long double x){
    long double out;
    __asm__ __volatile__("fldt %1\n\tfcos\n\tfstpt %0"
        : "=m"(out) : "m"(x) : "st");
    return out;
}
