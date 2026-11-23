#ifndef VMS_FOR_M112_H
#define VMS_FOR_M112_H
#include <stddef.h>
#include <stdint.h>
/* A native IA-32 FORTRAN RTL subset. None of these status values claim to
 * be VMS SS$_ statuses. Real RMS, condition frames, and VAX argument lists
 * remain UNSUPPORTED. I/O calls succeed only on a confirmed backend action.
 */
enum { PORT112_OK=1, PORT112_BADARG=2, PORT112_MODE=4,
       PORT112_NO_BACKEND=6, PORT112_STATE=8, PORT112_RANGE=10,
       PORT112_BUSY=12, PORT112_IO_ERROR=14, PORT112_OBJ_FMT=0x100u };
/* Statement ordinals from RTL/SRC/FORIOBEG.B32 STMT_ARG, STMT_ATR, ERR_ADR_IDX. */
enum {PORT112_WSF=1,PORT112_RSF=2,PORT112_WSU=3,PORT112_RSU=4,
      PORT112_WDF=5,PORT112_RDF=6,PORT112_WDU=7,PORT112_RDU=8,
      PORT112_WSL=9,PORT112_RSL=10,PORT112_WMF=11,PORT112_RMF=12,
      PORT112_WXF=13,PORT112_RKF=14,PORT112_WXU=15,PORT112_RKU=16,
      PORT112_WIF=17,PORT112_RIF=18,PORT112_WSN=19,PORT112_RSN=20};
/* Logical LUB mode flags, not ABI bit positions in original VAX LUBDEF. */
enum {PORT112_READ_ONLY=1u<<0,PORT112_DIRECT=1u<<1,
      PORT112_FORMATTED=1u<<2,PORT112_UNFORMATTED=1u<<3,
      PORT112_SEQUENTIAL=1u<<4,PORT112_KEYED=1u<<5,
      PORT112_APPEND=1u<<6,PORT112_NOTSEQORG=1u<<7};
enum port112_action { PORT112_ACT_OPEN=1,PORT112_ACT_BEGIN=2,
 PORT112_ACT_END=3,PORT112_ACT_BACKSPACE=4,PORT112_ACT_CLOSE=5,
 PORT112_ACT_DELETE=6,PORT112_ACT_ENDFILE=7,PORT112_ACT_REWIND=8,
 PORT112_ACT_UNLOCK=9 };
struct port112_backend {
    void *context;
    /* Return PORT112_OK only after the actual operation happened. */
    int (*operate)(void *context,enum port112_action action,
                   uint32_t unit,uint32_t flags,uint32_t record);
};
struct port112_unit {
    uint32_t number;
    uint32_t attributes;
    uint32_t record_number;
    uint32_t maximum_records;
    uint32_t record_bytes;
    void *associated_variable;
    uint32_t associated_bytes;
    uint32_t statement_kind;
    uint32_t current_args;
    uint32_t err_arg_index;
    uint32_t opened;
    uint32_t in_statement;
    uint32_t last_iostat;
};
int port112_unit_init(struct port112_unit *unit,uint32_t number);
int port112_io_begin(struct port112_unit *unit,
                     const struct port112_backend *backend,uint32_t stmt);
int port112_io_end(struct port112_unit *unit,const struct port112_backend *backend);
int port112_associated(struct port112_unit *unit);
int port112_backspace(struct port112_unit *unit,const struct port112_backend *backend);
int port112_close(struct port112_unit *unit,const struct port112_backend *backend);
int port112_define(struct port112_unit *unit,uint32_t max_records,
                   uint32_t record_words,void *associated,unsigned associated_bytes);
int port112_delete(struct port112_unit *unit,const struct port112_backend *backend,int direct);
int port112_endfile(struct port112_unit *unit,const struct port112_backend *backend);
int port112_rewind(struct port112_unit *unit,const struct port112_backend *backend);
int port112_unlock(struct port112_unit *unit,const struct port112_backend *backend);
uint32_t port112_statement_argument_flags(uint32_t kind);
uint32_t port112_statement_error_arg_index(uint32_t kind);
uint32_t port112_statement_forbidden_modes(uint32_t kind);
/* Source-specific native symbols (each in its own translation unit). */
int port112_fordecomf(struct port112_unit *,const struct port112_backend *);
int port112_fordecomo(struct port112_unit *,const struct port112_backend *);
int port112_forencomf(struct port112_unit *,const struct port112_backend *);
int port112_forencomo(struct port112_unit *,const struct port112_backend *);
int port112_forreaddf(struct port112_unit *,const struct port112_backend *);
int port112_forreaddo(struct port112_unit *,const struct port112_backend *);
int port112_forreaddu(struct port112_unit *,const struct port112_backend *);
int port112_forreadif(struct port112_unit *,const struct port112_backend *);
int port112_forreadio(struct port112_unit *,const struct port112_backend *);
int port112_forreadkf(struct port112_unit *,const struct port112_backend *);
int port112_forreadko(struct port112_unit *,const struct port112_backend *);
int port112_forreadku(struct port112_unit *,const struct port112_backend *);
int port112_forreadsf(struct port112_unit *,const struct port112_backend *);
int port112_forreadsl(struct port112_unit *,const struct port112_backend *);
int port112_forreadsn(struct port112_unit *,const struct port112_backend *);
int port112_forreadso(struct port112_unit *,const struct port112_backend *);
int port112_forreadsu(struct port112_unit *,const struct port112_backend *);
int port112_forrewrsf(struct port112_unit *,const struct port112_backend *);
int port112_forrewrso(struct port112_unit *,const struct port112_backend *);
int port112_forrewrsu(struct port112_unit *,const struct port112_backend *);
int port112_forwritdf(struct port112_unit *,const struct port112_backend *);
int port112_forwritdo(struct port112_unit *,const struct port112_backend *);
int port112_forwritdu(struct port112_unit *,const struct port112_backend *);
int port112_forwritif(struct port112_unit *,const struct port112_backend *);
int port112_forwritio(struct port112_unit *,const struct port112_backend *);
int port112_forwritsf(struct port112_unit *,const struct port112_backend *);
int port112_forwritsl(struct port112_unit *,const struct port112_backend *);
int port112_forwritsn(struct port112_unit *,const struct port112_backend *);
int port112_forwritso(struct port112_unit *,const struct port112_backend *);
int port112_forwritsu(struct port112_unit *,const struct port112_backend *);
int port112_foriobeg(struct port112_unit *,const struct port112_backend *,uint32_t);
int port112_forioend(struct port112_unit *,const struct port112_backend *);
int port112_forassoc(struct port112_unit *);
int port112_forbacksp(struct port112_unit *,const struct port112_backend *);
int port112_forclose(struct port112_unit *,const struct port112_backend *);
int port112_fordefine_w(struct port112_unit *,uint32_t,uint32_t,void *);
int port112_fordefine_l(struct port112_unit *,uint32_t,uint32_t,void *);
int port112_fordelete(struct port112_unit *,const struct port112_backend *);
int port112_fordelete_d(struct port112_unit *,const struct port112_backend *);
int port112_forendfil(struct port112_unit *,const struct port112_backend *);
int port112_forrewind(struct port112_unit *,const struct port112_backend *);
int port112_forunlock(struct port112_unit *,const struct port112_backend *);

#endif
