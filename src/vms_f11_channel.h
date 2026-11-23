#ifndef VMS37_F11_CHANNEL_H
#define VMS37_F11_CHANNEL_H
#include "vms_f11_ioc.h"
/* M185-M192. Read-only privileged source FIL$READVBN / IOC assignment
 * interface. This is NOT VMS process $ASSIGN, ACP or DQA0 public $QIO.
 * Every call executes at supervisor boot privilege only. */
#define PORT_F11_CHAN_SLOTS 4u
#define PORT_F11_CHAN_MAX_GENERATION 0x00ffffffu
enum { PORT_F11_CHAN_OK=1u, PORT_F11_CHAN_BAD=2u,
       PORT_F11_CHAN_BUSY=4u, PORT_F11_CHAN_NOCHANNEL=6u,
       PORT_F11_CHAN_EOF=8u, PORT_F11_CHAN_IO=10u,
       PORT_F11_CHAN_DIFFERENT=12u };
struct port_f11_chan_slot {
 struct port_f11_ioc_file file;
 uint32_t owner,generation;
 uint8_t assigned;
};
struct port_f11_chan_table {
 const struct port_f11_chain *chain;
 struct port_dq_ioc_queue *queue;
 uint32_t length,next_generation,open_count,assignments,releases;
 struct port_f11_chan_slot slots[PORT_F11_CHAN_SLOTS];
};
struct port_f11_chan_status {
 uint32_t owner,handle,position,length,physical_reads;
};
uint32_t port_f11_chan_init(struct port_f11_chan_table *,
 const struct port_f11_chain *,struct port_dq_ioc_queue *,uint32_t);
uint32_t port_f11_chan_validate(const struct port_f11_chan_table *);
uint32_t port_f11_chan_assign(struct port_f11_chan_table *,uint32_t,uint32_t *);
uint32_t port_f11_chan_release(struct port_f11_chan_table *,uint32_t,uint32_t);
uint32_t port_f11_chan_seek(struct port_f11_chan_table *,uint32_t,uint32_t,uint32_t);
uint32_t port_f11_chan_read(struct port_f11_chan_table *,uint32_t,uint32_t,
 uint8_t *,uint32_t,size_t,uint32_t *);
uint32_t port_f11_chan_compare(struct port_f11_chan_table *,uint32_t,uint32_t,
 const uint8_t *,uint32_t,uint32_t *);
uint32_t port_f11_chan_status(struct port_f11_chan_table *,uint32_t,uint32_t,
 struct port_f11_chan_status *);
#endif
