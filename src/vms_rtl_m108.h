#ifndef VMS_RTL_M108_H
#define VMS_RTL_M108_H
#include <stdint.h>
#include <stddef.h>
#include "vms_rtl_m107.h"
/* All functions operate on native IA-32 data, not a VAX CPU emulator.
 * An explicit PARTIAL result rejects unimplemented operand domains;
 * callers must never treat it as a valid VMS result. */
enum { PORT108_OK=1, PORT108_BAD_ARG=2, PORT108_PARTIAL=4,
       PORT108_OVERFLOW=6, PORT108_RESERVED=8, PORT108_DIVZERO=10,
       PORT108_EMPTY=12, PORT108_ONE=3, PORT108_BUSY=14 };
enum port108_rounding { PORT108_TRUNC, PORT108_FLOOR, PORT108_NEAREST, PORT108_ABS };
int port108_float_transform(enum port_m107_fmt f,const uint16_t *in,uint16_t *out,enum port108_rounding how);
int port108_float_integer(enum port_m107_fmt f,const uint16_t *in,int32_t *out);
int port108_float_from_integer(enum port_m107_fmt f,int32_t value,uint16_t *out);
int port108_float_mod(enum port_m107_fmt f,const uint16_t *a,const uint16_t *b,uint16_t *out);
int port108_float_dim(enum port_m107_fmt f,const uint16_t *a,const uint16_t *b,uint16_t *out);
int port108_float_product(enum port_m107_fmt f,const uint16_t a[2],const uint16_t b[2],uint16_t *out);
int port108_float_cabs(enum port_m107_fmt f,const uint16_t *pair,uint16_t *out);
int port108_integer_abs16(int16_t a,int16_t *out);
int port108_integer_abs32(int32_t a,int32_t *out);
int port108_integer_dim16(int16_t a,int16_t b,int16_t *out);
int port108_integer_dim32(int32_t a,int32_t b,int32_t *out);
int port108_integer_mod16(int16_t a,int16_t b,int16_t *out);
int port108_integer_mod32(int32_t a,int32_t b,int32_t *out);
int port108_math_bit16(unsigned op,uint16_t a,uint16_t b,uint16_t *out);
int port108_math_bit32(unsigned op,uint32_t a,uint32_t b,uint32_t *out);
int port108_random_mth(uint32_t *seed,uint16_t out[2]);
int port108_random_fortran(uint16_t *high,uint16_t *low,uint16_t out[2]);
/* Source queue interlock instructions: a validated window of 32-bit pointers.
 * The result for a newly created or newly emptied queue is PORT108_ONE.
 * No atomic multiprocessor interlock is claimed (still partial). */
struct port108_queue {uint8_t *mem;size_t bytes;uint32_t base,head;};
int port108_queue_init(struct port108_queue *q);
int port108_queue_insert(struct port108_queue *q,uint32_t entry,int at_tail);
int port108_queue_remove(struct port108_queue *q,uint32_t *removed,int at_tail);
int port108_movq(const uint8_t *src,uint8_t *dst);
int port108_movtc(const uint8_t *src,size_t ns,const uint8_t table[256],uint8_t fill,uint8_t *dest,size_t nd);
int port108_movtuc(const uint8_t *src,size_t ns,const uint8_t table[256],uint8_t escape,uint8_t fill,uint8_t *dest,size_t nd,size_t *index);
uint32_t port108_crc_reference(uint32_t polynomial,uint32_t initial,const uint8_t *src,size_t bytes);
int port108_emul(int32_t a,int32_t b,int32_t add,int64_t *result);
int port108_ediv(int32_t divisor,int64_t dividend,int32_t *quot,int32_t *rem);
int port108_get_opcode(const uint8_t *instruction,size_t readable,uint16_t *opcode);
/* The wrappers correspond to original, distinct modules, not complete VAX ABIs. */
#endif
