#ifndef VMS37_NATIVE_F11_IOC_H
#define VMS37_NATIVE_F11_IOC_H
#include "vms_dq_ioc.h"
#include "vms_f11_chain.h"
/* SYS/SRC/FILEREAD.MAR FIL$READVBN -> FIL$RDWRTLBN ->
 * EXE$INSIOQ / DQ_STARTIO / IOCIOPOST. Private privileged read-only
 * bridge, NOT a published DQA0, process $QIO, interrupt, AST or ACP. */
#define PORT_F11_IOC_MAX_READ 2048u
enum { PORT_F11_IOC_OK=1u, PORT_F11_IOC_BAD=2u,
       PORT_F11_IOC_EOF=4u, PORT_F11_IOC_IO=6u,
       PORT_F11_IOC_BUSY=8u,PORT_F11_IOC_MAP=10u,
       PORT_F11_IOC_DIFFERENT=12u };
struct port_f11_ioc_file {
 const struct port_f11_chain *chain;
 struct port_dq_ioc_queue *queue;
 uint32_t position, length, next_tag, reads;
 uint32_t pending_tag, pending_within;
 uint8_t pending, sector[512];
};
uint32_t port_f11_ioc_init(struct port_f11_ioc_file *,
 const struct port_f11_chain *,struct port_dq_ioc_queue *,uint32_t);
uint32_t port_f11_ioc_validate(const struct port_f11_ioc_file *);
uint32_t port_f11_ioc_seek(struct port_f11_ioc_file *,uint32_t);
uint32_t port_f11_ioc_locate(const struct port_f11_ioc_file *,uint32_t *,uint32_t *);
uint32_t port_f11_ioc_stage(struct port_f11_ioc_file *);
uint32_t port_f11_ioc_post(struct port_f11_ioc_file *);
uint32_t port_f11_ioc_read(struct port_f11_ioc_file *,uint8_t *,uint32_t,
 size_t,uint32_t *);
uint32_t port_f11_ioc_compare(struct port_f11_ioc_file *,const uint8_t *,
 uint32_t,uint32_t *);
#endif
