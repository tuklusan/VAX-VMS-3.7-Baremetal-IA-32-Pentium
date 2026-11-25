#include "vms_sysboot_elf.h"
#include "vms_iniloa_entry.h"
#include "vms_ini_pc_bus.h"
#include "vms_sys_executive_entry.h"
#include "vms_v37_dyn_tags.h"
#include "vms_sysboot_map.h"
#include "vms_sysloa386.h"
/* Native VMS 3.7 port research monitor. NOT VAX/VMS or a VMS implementation.
 * It uses VGA text memory and 8042 keyboard-controller I/O directly.
 * BIOS is only used by the stage-1 loader before entering protected mode.
 */
#include <stdint.h>
#include <stddef.h>
#include "vms_ast.h"
#include "vms_astdel.h"
#include "vms_postef.h"
#include "vms_mutex.h"
#include "vms_hpwd.h"
#include "vms_validate.h"
#include "vms_logintime.h"
#include "vms_uaf_v37.h"
#include "vms_login_fileio.h"
#include "vms_logincmd.h"
#include "vms_login_interact.h"
#include "vms_dcl_exit.h"
#include "vms_dcl_status.h"
#include "vms_dcl_if.h"
#include "vms_dcl_inquire.h"
#include "vms_dcl_on.h"
#include "vms_dcl_goto.h"
#include "vms_dcl_descr.h"
#include "vms_dcl_readrec.h"
#include "vms_dcl_initial.h"
#include "vms_dcl_command.h"
#include "vms_rem_protocol.h"
extern int port_rem_protocol_guest_selftest(void);
#include "vms_dcl_intimages.h"
#include "vms_login_uart.h"
#include "vms_boot_readprompt.h"
#include "vms_initvec.h"
#include "vms_booparam.h"
#include "vms_bootio.h"
#include "vms_boot_files11.h"
#include "vms_vmbargs.h"
#include "vms_ini_sysparams.h"
#include "vms_vmb_rpb.h"
#include "vms_condition.h"
#include "vms_ne2k.h"
#include "vms_decnet.h"
#include "vms_nsp.h"
#include "vms_nsp_link.h"
#include "vms_session.h"
#include "vms_nsp_data.h"
#include "vms_nsp_reliable.h"
#include "vms_mirror.h"
#include "vms_mirror_server.h"
#include "vms_serial.h"
#include "vms_qio.h"
#include "vms_channels.h"
#include "vms_dq_crb.h"
#include "vms_ata_x86.h"
#include "vms_dq_ioc.h"
#include "vms_dq_ioc_write.h"
#include "vms_f11_acp87.h"
#include "vms_ioc_phys.h"
#include "vms_exe_np_pool.h"

#define VGA ((volatile uint16_t *)0xB8000)
#define WIDTH 80
#define HEIGHT 25
extern void arch_init(void);
extern int arch_vms_ring_probe(void);
extern uint32_t arch_a20_ready(void);
extern uint32_t arch_ram_window_ready(void);
extern uint32_t arch_ticks(void);
extern int arch_adopt_executive_clock(const struct port_exe_systime *,
                                     void (*tick)(void));
extern int arch_adopt_vms_scheduler(struct port_v37_scheduler *,
    port_v37_sched_interrupt_fn);
extern uint32_t arch_iopost_bind(uint32_t (*)(void));
extern uint32_t arch_iopost_trigger(void);
extern uint32_t arch_iopost_count(void);
extern uint32_t arch_iopost_last_status(void);
extern const struct port_exe_systime *arch_exe_systime(void);
extern uint32_t arch_last_vector(void);
extern void arch_probe_int3(void);
extern void arch_probe_ud2(void);
extern void arch_run_user_test(int fault);
extern void arch_run_userptr_test(void);
extern void arch_run_user_channel_test(void);
extern void arch_run_user_qio_test(void);
extern void arch_run_user_qio_cancel_test(void);
extern void arch_run_user_qio_teardown_test(void);
extern void arch_run_user_copyin_test(void);
extern void arch_run_user_qio_desc_test(void);
extern uint32_t arch_user_desc_result(void);
extern uint32_t arch_user_desc_calls(void);
extern uint32_t arch_user_copyin_flags(void);
extern uint32_t arch_user_copyin_successes(void);
extern uint32_t arch_user_copyin_matched(void);
extern uint32_t arch_user_read_result(void),arch_user_read_count(void),arch_user_read_bytes(void);
extern uint32_t arch_user_cancel_result(void),arch_user_cancel_calls(void);
extern uint32_t arch_user_teardown_result(void);
extern uint32_t arch_user_channel_result(void),arch_user_channel_calls(void);
extern uint32_t arch_user_channel_assigned(void),arch_user_channel_deassigned(void);
extern uint32_t arch_user_ptr_result(void),arch_user_ptr_calls(void);
extern void arch_run_user_ast(uint32_t parameter);
extern uint32_t arch_user_ast_value(void);
extern uint32_t arch_user_ast_mode(void);
extern uint32_t arch_user_cpl(void);
extern uint32_t arch_user_syscalls(void);
extern uint32_t arch_user_faults(void);
extern uint32_t arch_user_exits(void);
extern void arch_run_scheduler_test(void);
extern uint32_t arch_sched_switches(void),arch_sched_reports(void);
extern uint32_t arch_sched_done(void),arch_sched_sequence(void);
extern uint32_t arch_sched_a(void),arch_sched_b(void);
extern void arch_run_preempt_test(void),arch_run_isolation_test(void);
extern uint32_t arch_sched_preemptions(void),arch_sched_auto_ast_reports(void);
extern uint32_t arch_sched_auto_ast_completed(void),arch_sched_ast_delivered(void);
extern uint32_t arch_isolation_faults(void),arch_isolation_intact(void);
extern void arch_run_vm_test(void);
extern uint32_t arch_vm_isolated(void), arch_vm_released(void);
extern void arch_run_demand_test(void);
extern void arch_run_event_test(void);
extern void arch_run_mutex_test(void);
extern uint32_t arch_mutex_report(unsigned);
extern uint32_t arch_mutex_waits(void);
extern uint32_t arch_mutex_wakes(void);
extern uint32_t arch_mutex_remaining(void);
extern uint32_t arch_mutex_held(void);
extern int arch_disk_irq_bind_crb(struct port_dq_crb *);
extern uint32_t arch_disk_irq_crb_count(void);
extern int port_ne2k_x86_probe(struct port_ne2k *nic);
extern int port_ne2k_x86_pending(struct port_ne2k *nic);
extern int port_ne2k_x86_send(struct port_ne2k *nic,const uint8_t *frame,unsigned len);
extern int port_ne2k_x86_recv(struct port_ne2k *nic,uint8_t *frame,unsigned capacity);
extern int port_ne2k_x86_set_station(struct port_ne2k *nic,const uint8_t mac[6]);
extern uint32_t arch_event_blocks(void),arch_event_wakes(void);
extern uint32_t arch_event_immediate(void),arch_event_flags(void);
extern uint32_t arch_event_report(unsigned);
extern uint32_t arch_demand_faults(void),arch_demand_zero_seen(void);
extern uint32_t arch_demand_isolated(void),arch_demand_reclaimed(void);
extern uint32_t arch_phys_free(void),arch_phys_high_water(void);
extern uint32_t arch_pcb_pid(unsigned),arch_pcb_ticks(unsigned);
extern uint32_t arch_pcb_dispatches(unsigned),arch_pcb_state(unsigned);
extern uint32_t arch_pcb_cr3(unsigned);
extern int arch_pcb_valid(void);
extern uint32_t arch_kstack_checks(void),arch_kstack_faults(void);
extern uint32_t arch_kstack_entries(unsigned),arch_kstack_tss_esp0(void);
extern int arch_kstack_protected(void);
struct port_condition_process portlab_conditions;
static struct port_condition_handler diagnostic_handler;
static struct port_condition_handler user_exception_handler;
static struct vms_ast_process ast_proc;
static struct vms_ast_process ring3_ast_proc;
static struct vms_ast_request ring3_ast_request;
static struct vms_ast_request ast_requests[12];
static unsigned ast_pool_cursor;
static unsigned return_mode=VMS_MODE_USER;
static unsigned cursor_row, cursor_col;
static uint8_t attribute = 0x07;
static struct port_ne2k nic_info;
static int nic_last_result=-1;
static uint8_t nic_buffer[1518];
static int nic_rx_result;
static uint16_t decnet_nodeid=0x0401u;
static int decnet_last_status=-1;
static struct port_decnet_short decnet_rx;
static struct port_decnet_hello decnet_peer;
static struct port_decnet_router_hello decnet_router_info;
static struct port_decnet_adjacencies decnet_neighbors;
static uint32_t decnet_router_seen,decnet_rx_last_tick;
static int decnet_enabled;
static uint32_t decnet_hello_last_tick,decnet_hello_sent,decnet_hello_received;
static void decnet_service(void);
static struct port_nsp_message nsp_last_packet;
static uint32_t nsp_parsed, nsp_rejected;
static struct port_nsp_link nsp_link;
static uint32_t nsp_tx_count, nsp_tx_errors;
static uint8_t nsp_payload[96];
static uint8_t nsp_tx_frame[1518];
static uint8_t nsp_replay[11u+PORT_NSP_DATA_MAX_BYTES];
static unsigned nsp_replay_len;
static struct port_nsp_reliable nsp_transport;
static uint32_t nsp_rx_partial_unsupported;
static uint8_t nsp_mirror_request[32];
static unsigned nsp_mirror_request_len;
static uint32_t nsp_mirror_ok,nsp_mirror_bad,nsp_mirror_timeouts;
static uint8_t nsp_mirror_waiting;
