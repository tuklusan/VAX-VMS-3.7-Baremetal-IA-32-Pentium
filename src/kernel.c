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
#include "vms_dq_ioc.h"
#include "vms_dq_ioc_write.h"
#include "vms_f11_acp87.h"
#include "vms_ioc_phys.h"

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
extern int port_ata_x86_read28(uint32_t lba,uint16_t dst[256]);
extern int port_ata_x86_write28(uint32_t lba,const uint16_t src[256]);
extern uint32_t port_ata_x86_irq_count(void);
extern void port_ata_x86_irq(void);
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
static struct port_mirror_server nsp_server;
static int nsp_server_enabled;
static struct port_nsp_data nsp_data;
static struct port_nsp_data_ack nsp_dataack;
static uint32_t nsp_data_received,nsp_data_rejected,nsp_dataacks_sent,nsp_dataacks_received;
static uint16_t nsp_data_last_sequence,nsp_data_tx_sequence;
static void hex8(uint8_t value);
static uint16_t ata_sector[256];
static uint16_t qio_sector[256];
static struct port_qio_service qio_service;
static struct port_channel_table qio_channels;
static uint32_t qio_last_channel;
static struct port_qio_request qio_request;
static struct port_qio_iosb qio_iosb;
static struct port_event_cluster qio_events;
static struct vms_ast_process qio_ast_process;
static uint32_t qio_ast_seen;
static void qio_completion_callback(void *ctx,uint32_t parameter) {
    (void)ctx;qio_ast_seen=parameter;
}
static int qio_ata_read(void *ctx,uint32_t lba,uint16_t words[256]) {
    (void)ctx;return port_ata_x86_read28(lba,words);
}

static uint32_t ata_reads,ata_last_checksum;
static int ata_last_result;
static int ata_test(void) {
    const char header[]="VMS37LAB-LBA0000";
    const uint32_t checksum[2]={65268u,65093u};
    ata_reads=ata_last_checksum=0;
    for (uint32_t lba=1;lba<=2;lba++) {
        int rc=port_ata_x86_read28(lba,ata_sector);
        if (rc) return (ata_last_result=rc);
        ++ata_reads;
        uint32_t sum=0;
        const uint8_t *bytes=(const uint8_t *)(const void *)ata_sector;
        for (unsigned i=0;i<512;i++) sum+=bytes[i];
        ata_last_checksum=sum;
        for (unsigned i=0;i<16;i++) {
            char expected=header[i];
            if (i==15) expected=(char)('0'+lba);
            if (bytes[i]!=(uint8_t)expected)
                return (ata_last_result=-5);
        }
        if (sum!=checksum[lba-1]) return (ata_last_result=-6);
    }
    return (ata_last_result=0);
}


static inline uint8_t inb(uint16_t port) {
    uint8_t ret;
    __asm__ volatile ("inb %1, %0" : "=a"(ret) : "Nd"(port));
    return ret;
}
static inline void outb(uint16_t port, uint8_t value) {
    __asm__ volatile ("outb %0, %1" : : "a"(value), "Nd"(port));
}
static void refresh_cursor(void) {
    uint16_t pos = (uint16_t)(cursor_row * WIDTH + cursor_col);
    outb(0x3D4, 0x0F); outb(0x3D5, (uint8_t)pos);
    outb(0x3D4, 0x0E); outb(0x3D5, (uint8_t)(pos >> 8));
}
static void clear_screen(void) {
    for (unsigned i = 0; i < WIDTH * HEIGHT; ++i)
        VGA[i] = (uint16_t)(attribute << 8) | ' ';
    cursor_row = cursor_col = 0;
    refresh_cursor();
}
static void scroll_if_needed(void) {
    if (cursor_row < HEIGHT) return;
    for (unsigned y = 1; y < HEIGHT; ++y)
        for (unsigned x = 0; x < WIDTH; ++x)
            VGA[(y - 1) * WIDTH + x] = VGA[y * WIDTH + x];
    for (unsigned x = 0; x < WIDTH; ++x)
        VGA[(HEIGHT - 1) * WIDTH + x] = (uint16_t)(attribute << 8) | ' ';
    cursor_row = HEIGHT - 1;
}
static void putc(char c) {
    port_serial_x86_putc(c);
    if (c == '\n') { ++cursor_row; cursor_col = 0; }
    else if (c == '\b') {
        if (cursor_col) --cursor_col;
        VGA[cursor_row * WIDTH + cursor_col] = (uint16_t)(attribute << 8) | ' ';
    } else if (c >= ' ' && c <= '~') {
        VGA[cursor_row * WIDTH + cursor_col] = (uint16_t)(attribute << 8) | (uint8_t)c;
        if (++cursor_col == WIDTH) { cursor_col = 0; ++cursor_row; }
    }
    scroll_if_needed();
    refresh_cursor();
}
static void puts(const char *s) { while (*s) putc(*s++); }
static int eq(const char *a, const char *b) {
    while (*a && *b && *a == *b) { ++a; ++b; }
    return *a == *b;
}
static void hex8(uint8_t n) {
    const char *hex="0123456789ABCDEF";
    putc(hex[n>>4]);
    putc(hex[n&15]);
}
static void hex32(uint32_t n) {
    for (unsigned shift = 28;; shift -= 4) {
        unsigned d = (n >> shift) & 15;
        putc((char)(d < 10 ? '0' + d : 'A' + d - 10));
        if (shift == 0) break;
    }
}
static const char scan_unshifted[0x40] = {
    [0x02]='1',[0x03]='2',[0x04]='3',[0x05]='4',[0x06]='5',
    [0x07]='6',[0x08]='7',[0x09]='8',[0x0A]='9',[0x0B]='0',
    [0x0C]='-',[0x0D]='=',[0x10]='Q',[0x11]='W',[0x12]='E',
    [0x13]='R',[0x14]='T',[0x15]='Y',[0x16]='U',[0x17]='I',
    [0x18]='O',[0x19]='P',[0x1A]='[',[0x1B]=']',[0x1E]='A',
    [0x1F]='S',[0x20]='D',[0x21]='F',[0x22]='G',[0x23]='H',
    [0x24]='J',[0x25]='K',[0x26]='L',[0x27]=';',[0x28]='\'',
    [0x2B]='\\',[0x2C]='Z',[0x2D]='X',[0x2E]='C',[0x2F]='V',
    [0x30]='B',[0x31]='N',[0x32]='M',[0x33]=',',[0x34]='.',
    [0x35]='/',[0x39]=' '
};
/* Simple set-1 scan-code polling. No IRQ or key-repeat handling. */
static char getkey_mode(int preserve_case) {
    for (;;) {
        int serial=preserve_case?port_serial_x86_getc_raw():port_serial_x86_getc();
        if(serial>=0)return (char)serial;
        if ((inb(0x64) & 1) == 0) { decnet_service(); continue; }
        uint8_t scan = inb(0x60);
        if (scan == 0x1C) return '\n';
        if (scan == 0x0E) return '\b';
        if (!(scan & 0x80) && scan < sizeof(scan_unshifted)) {
            char c = scan_unshifted[scan];
            if (c) return c;
        }
    }
}
static __attribute__((unused)) char getkey(void) { return getkey_mode(0); }
static void show_cpu(void) {
    uint32_t a, b, c, d;
    char vendor[13];
    __asm__ volatile ("cpuid" : "=a"(a), "=b"(b), "=c"(c), "=d"(d) : "a"(0));
    for (unsigned i = 0; i < 4; ++i) {
        vendor[i] = (char)(b >> (i * 8));
        vendor[4+i] = (char)(d >> (i * 8));
        vendor[8+i] = (char)(c >> (i * 8));
    }
    vendor[12] = 0;
    puts("CPUID vendor: "); puts(vendor); putc('\n');
    __asm__ volatile ("cpuid" : "=a"(a), "=b"(b), "=c"(c), "=d"(d) : "a"(1));
    puts("CPUID EAX: 0x"); hex32(a); putc('\n');
}
static int begins(const char *text, const char *prefix) {
    while (*prefix) { if (*text++ != *prefix++) return 0; }
    return 1;
}
static void show_clock(void) {
    puts("PIT IRQ0 ticks (~100Hz): 0x");
    hex32(arch_ticks()); putc('\n');
}
static void show_ast(void) {
    puts("AST pid 0x"); hex32(ast_proc.pid);
    puts(" lvl="); putc((char)('0'+ast_proc.ast_level));
    puts(" IPL="); hex32(ast_proc.software_ipl);
    puts(" pending=0x"); hex32(ast_proc.queued);
    puts(" delivered=0x"); hex32(ast_proc.delivered); putc('\n');
    puts("Enabled mask=0x"); hex32(ast_proc.enable_mask);
    puts(" active mask=0x"); hex32(ast_proc.active_mask); putc('\n');
    puts("Current simulated return mode (0..3): ");
    putc((char)('0'+return_mode)); putc('\n');
    puts("AST callbacks execute in the CPL0 monitor, not a VMS process.\n");
}
static void ast_callback(void *context, uint32_t parameter) {
    (void)context;
    puts("[native AST callback, parameter=0x");
    hex32(parameter);
    puts("]\n");
}
static int queue_ast(unsigned kind, unsigned mode) {
    struct vms_ast_request *r=0;
    for (unsigned i=0;i<12;++i) {
        unsigned idx=(ast_pool_cursor+i)%12;
        if (!ast_requests[idx].state) {
            r=&ast_requests[idx];
            ast_pool_cursor=(idx+1)%12;
            break;
        }
    }
    if (!r) return -1;
    r->next=0;
    r->pid=ast_proc.pid;
    r->parameter=ast_proc.delivered+ast_proc.queued+1;
    r->callback=ast_callback;
    r->context=0;
    r->mode=(uint8_t)mode;
    r->kind=(uint8_t)kind;
    r->state=0;
    return vms_ast_enqueue(&ast_proc,r);
}
static __attribute__((unused)) void ast_safe_point(void) {
    /* Return-to-process trampoline pending: this dispatch is called from the
     * monitor safe point, and never runs inside the hardware IRQ0 handler. */
    for (unsigned i=0; i<12; ++i) {
        int rc=vms_ast_dispatch_one(&ast_proc,return_mode);
        if (rc==VMS_AST_EMPTY || rc==VMS_AST_DEFERRED) break;
        if (rc!=VMS_AST_OK) {
            puts("AST completion invariant failed. Halting.\n");
            for (;;) __asm__ volatile("cli; hlt");
        }
    }
}
static enum port_condition_action diagnostic_condition_handler(
    const struct port_condition *condition, void *context) {
    (void)context;
    return condition->continuable ? PORT_COND_CONTINUE : PORT_COND_RESIGNAL;
}
static void show_conditions(void) {
    puts("Signals=0x"); hex32(portlab_conditions.signals);
    puts(" handled=0x"); hex32(portlab_conditions.handled);
    puts(" fatal=0x"); hex32(portlab_conditions.fatal); putc('\n');
    puts("Last: "); puts(port_condition_name(portlab_conditions.last.type));
    puts(" vector=0x"); hex32(portlab_conditions.last.vector);
    puts(" EIP=0x"); hex32(portlab_conditions.last.eip); putc('\n');
}
void arch_report_fault(uint32_t vector, uint32_t error, uint32_t eip) {
    puts("\n*** x86 fault: vector 0x"); hex32(vector);
    puts(" error 0x"); hex32(error); puts(" EIP 0x"); hex32(eip); putc('\n');
}
static int decnet_hello_send(void) {
    const struct port_decnet_router_entry *router=
        port_decnet_adj_best(&decnet_neighbors,decnet_nodeid);
    int n=port_decnet_build_endnode_hello_to(nic_buffer,sizeof nic_buffer,
                decnet_nodeid,576,15,router?router->hello.mac:0);
    if(n<0)return n;
    int rc=port_ne2k_x86_send(&nic_info,nic_buffer,(unsigned)n);
    if(rc==0)++decnet_hello_sent;
    return rc;
}
static int nsp_send_probe(uint8_t flags) {
    if(nic_last_result!=0)return -1;
    /* Numbered-object Session Control request for the DECnet MIRROR object.
     * This is only a connection probe: no Mirror DATA exchange yet. */
    uint8_t session[PORT_SESSION_REQUEST_BYTES];
    int s=port_session_build_request(session,sizeof session,
                                      PORT_SESSION_MIRROR_OBJECT);
    if(s<0)return s;
    int n=port_nsp_build_connect(nsp_payload,sizeof nsp_payload,flags,
                                 nsp_link.local_link,0,576,
                                 session,(unsigned)s);
    if(n<0)return n;
    int wire=port_decnet_build_short(nic_buffer,sizeof nic_buffer,
                   nsp_link.local_node,nsp_link.remote_node,nsp_payload,(unsigned)n);
    if(wire<0)return wire;
    int rc=port_ne2k_x86_send(&nic_info,nic_buffer,(unsigned)wire);
    if(rc==0)++nsp_tx_count;else ++nsp_tx_errors;
    return rc;
}
/* One copy of an NSP frame survives until its DATA is acknowledged.
 * Send from a distinct buffer: RX payloads point inside nic_buffer. */
static int nsp_send_nodes(uint16_t local,uint16_t peer,
                          const uint8_t *payload,unsigned length) {
    int frame=port_decnet_build_short(nsp_tx_frame,sizeof nsp_tx_frame,
                                       local,peer,payload,length);
    if(frame<=0)return -1;
    int rc=port_ne2k_x86_send(&nic_info,nsp_tx_frame,(unsigned)frame);
    if(rc==0)++nsp_tx_count;else ++nsp_tx_errors;
    return rc;
}
static int nsp_send_segment(const uint8_t *payload,unsigned length) {
    return nsp_send_nodes(nsp_link.local_node,nsp_link.remote_node,
                          payload,length);
}
static int nsp_send_data(const uint8_t *body,unsigned length) {
    if(nsp_link.state!=PORT_NSP_CONNECTED||!body||length==0 ||
       length>PORT_NSP_DATA_MAX_BYTES)return -1;
    uint16_t sequence;
    if(port_nsp_tx_begin(&nsp_transport,arch_ticks(),&sequence)<0)
        return -2; /* stop-and-wait; do not overwrite unacknowledged DATA */
    int n=port_nsp_data_build(nsp_replay,sizeof nsp_replay,
        PORT_NSP_DATA_MOM,nsp_link.remote_link,nsp_link.local_link,
        nsp_transport.have_rx?nsp_transport.last_rx:0u,0,sequence,
        body,length);
    if(n<=0){port_nsp_tx_cancel(&nsp_transport);return -1;}
    nsp_replay_len=(unsigned)n;
    int rc=nsp_send_segment(nsp_replay,nsp_replay_len);
    if(rc!=0){port_nsp_tx_cancel(&nsp_transport);nsp_replay_len=0;}
    if(rc==0)nsp_data_tx_sequence=sequence;
    return rc;
}
static void nsp_send_data_ack(void) {
    int a=port_nsp_data_ack_build(nsp_payload,sizeof nsp_payload,
        nsp_link.remote_link,nsp_link.local_link,
        nsp_transport.have_rx?nsp_transport.last_rx:0u,0);
    if(a>0&&nsp_send_segment(nsp_payload,(unsigned)a)==0)
        ++nsp_dataacks_sent;
}
static void nsp_note_peer_ack(uint16_t sequence) {
    int rc=port_nsp_tx_ack(&nsp_transport,sequence);
    if(rc==1){++nsp_dataacks_received;nsp_replay_len=0;}
    else if(rc<0||rc==0)++nsp_data_rejected;
}
/* Passive object-25 service on one isolated Phase IV link. */
static int nsp_server_process_short(const struct port_decnet_short *pkt) {
    if(!nsp_server_enabled||pkt->src_node==decnet_nodeid)return 0;
    struct port_nsp_message ctrl={0};
    if(port_nsp_parse(pkt->payload,pkt->payload_len,&ctrl)==0) {
        if(ctrl.flags==PORT_NSP_CI||ctrl.flags==PORT_NSP_RCI) {
            int accepted=port_mirror_server_connect(&nsp_server,decnet_nodeid,
                                                     pkt->src_node,&ctrl);
            if(accepted>0) {
                /* CA followed by CC.  Accept data = 2-byte max block size.
                 * Full VMS session credentials/objects remain unsupported. */
                int a=port_nsp_build_conn_ack(nsp_payload,sizeof nsp_payload,
                                              nsp_server.peer_link);
                if(a>0)(void)nsp_send_nodes(decnet_nodeid,pkt->src_node,
                                            nsp_payload,(unsigned)a);
                static const uint8_t accept_data[3]={2,0,2}; /* 512 */
                int c=port_nsp_build_connect(nsp_payload,sizeof nsp_payload,
                      PORT_NSP_CC,nsp_server.local_link,nsp_server.peer_link,
                      512,accept_data,sizeof accept_data);
                if(c>0)(void)nsp_send_nodes(decnet_nodeid,pkt->src_node,
                                            nsp_payload,(unsigned)c);
            } else if(ctrl.source_link) {
                /* Object not supported or diagnostic listener busy. */
                int d=port_nsp_build_disconnect(nsp_payload,sizeof nsp_payload,
                      PORT_NSP_DI,ctrl.source_link,PORT_MIRROR_SERVER_LINK,4);
                if(d>0)(void)nsp_send_nodes(decnet_nodeid,pkt->src_node,
                                            nsp_payload,(unsigned)d);
            }
            return 1;
        }
        if(ctrl.flags==PORT_NSP_DI&&
           port_mirror_server_matches(&nsp_server,pkt->src_node,
                    ctrl.destination_link,ctrl.source_link)) {
            int d=port_nsp_build_disconnect(nsp_payload,sizeof nsp_payload,
                PORT_NSP_DC,nsp_server.peer_link,nsp_server.local_link,42);
            if(d>0)(void)nsp_send_nodes(decnet_nodeid,pkt->src_node,
                                        nsp_payload,(unsigned)d);
            port_mirror_server_close(&nsp_server);
            return 1;
        }
    }
    if(!nsp_server.active||pkt->src_node!=nsp_server.peer_node)return 0;
    struct port_nsp_data data;
    if(port_nsp_data_parse(pkt->payload,pkt->payload_len,&data)==0&&
       port_mirror_server_matches(&nsp_server,pkt->src_node,
                               data.destination_link,data.source_link)) {
        int result=port_mirror_server_data(&nsp_server,pkt->src_node,
                         pkt->payload,pkt->payload_len,arch_ticks());
        if(result>=0||result==PORT_MIRROR_SERVER_ORDER) {
            int a=port_nsp_data_ack_build(nsp_payload,sizeof nsp_payload,
                nsp_server.peer_link,nsp_server.local_link,
                nsp_server.transport.have_rx?
                    nsp_server.transport.last_rx:0u,0);
            if(a>0)(void)nsp_send_nodes(decnet_nodeid,pkt->src_node,
                                        nsp_payload,(unsigned)a);
            if(result>0)(void)nsp_send_nodes(decnet_nodeid,pkt->src_node,
                                  nsp_server.replay,nsp_server.replay_len);
        }
        return 1;
    }
    struct port_nsp_data_ack ack;
    if(port_nsp_data_ack_parse(pkt->payload,pkt->payload_len,&ack)==0&&
       port_mirror_server_matches(&nsp_server,pkt->src_node,
                               ack.destination_link,ack.source_link)) {
        (void)port_mirror_server_ack(&nsp_server,pkt->src_node,
                                     pkt->payload,pkt->payload_len);
        return 1;
    }
    return 0;
}
static void nsp_process_short(const struct port_decnet_short *shortpkt) {
    if(shortpkt->dst_node!=decnet_nodeid)return;
    if(nsp_server_process_short(shortpkt))return;
    int ns=port_nsp_parse(shortpkt->payload,shortpkt->payload_len,
                          &nsp_last_packet);
    if(ns==PORT_NSP_OK) {
        ++nsp_parsed;
        enum port_nsp_state previous=nsp_link.state;
        (void)port_nsp_link_rx(&nsp_link,shortpkt->src_node,
                               shortpkt->dst_node,&nsp_last_packet);
        if(previous!=PORT_NSP_CONNECTED &&
           nsp_link.state==PORT_NSP_CONNECTED) {
            port_nsp_reliable_init(&nsp_transport);
            nsp_replay_len=0;nsp_mirror_waiting=0;
        }
    } else if(nsp_link.state==PORT_NSP_CONNECTED &&
              shortpkt->src_node==nsp_link.remote_node &&
              port_nsp_data_parse(shortpkt->payload,shortpkt->payload_len,&nsp_data)==0) {
        if(nsp_data.destination_link!=nsp_link.local_link||
           nsp_data.source_link!=nsp_link.remote_link) {
            ++nsp_data_rejected;return;
        }
        /* No record fragmentation/reassembly: only full MOM packets. */
        if(nsp_data.flags!=PORT_NSP_DATA_MOM) {
            ++nsp_rx_partial_unsupported;++nsp_data_rejected;return;
        }
        /* DATA embeds a cumulative ACK for our outbound segment. */
        if(nsp_transport.pending &&
           nsp_data.ack_data==nsp_transport.pending_seq)
            nsp_note_peer_ack(nsp_data.ack_data);
        int verdict=port_nsp_rx_sequence(&nsp_transport,nsp_data.sequence);
        if(verdict==PORT_NSP_RX_NEW) {
            ++nsp_data_received;
            nsp_data_last_sequence=nsp_data.sequence;
            if(nsp_mirror_waiting) {
                int result=port_mirror_verify(nsp_mirror_request,
                    nsp_mirror_request_len,nsp_data.body,nsp_data.body_len);
                if(result==0)++nsp_mirror_ok;
                else ++nsp_mirror_bad;
                nsp_mirror_waiting=0;
            }
        } else if(verdict!=PORT_NSP_RX_DUPLICATE) ++nsp_data_rejected;
        /* Duplicate and unexpected DATA both request last good ACK.
         * No application callback for duplicates. */
        nsp_send_data_ack();
    } else if(nsp_link.state==PORT_NSP_CONNECTED &&
              shortpkt->src_node==nsp_link.remote_node &&
              port_nsp_data_ack_parse(shortpkt->payload,shortpkt->payload_len,
                                      &nsp_dataack)==0) {
        if(nsp_dataack.destination_link==nsp_link.local_link&&
           nsp_dataack.source_link==nsp_link.remote_link)
            nsp_note_peer_ack(nsp_dataack.ack_data);
        else ++nsp_data_rejected;
    } else {++nsp_rejected;++nsp_data_rejected;}
}
static void decnet_service(void) {
    if(!decnet_enabled||nic_last_result!=0)return;
    uint32_t now=arch_ticks();
    if((uint32_t)(now-decnet_rx_last_tick)>=10u) {
        decnet_rx_last_tick=now;
        port_decnet_adj_expire(&decnet_neighbors,now);
        /* Poll a single frame at a safe point, never inside IRQ0. */
        int n=port_ne2k_x86_recv(&nic_info,nic_buffer,sizeof nic_buffer);
        if(n>0) {
            if(port_decnet_parse_router_hello(nic_buffer,(unsigned)n,
                                                &decnet_router_info)==0) {
                if(port_decnet_adj_update(&decnet_neighbors,&decnet_router_info,
                                           now)==0)++decnet_router_seen;
            } else if(port_decnet_parse_endnode_hello(nic_buffer,(unsigned)n,
                                                      &decnet_peer)==0) {
                ++decnet_hello_received;
            } else if(port_decnet_parse_short(nic_buffer,(unsigned)n,&decnet_rx)==0 &&
                      decnet_rx.dst_node==decnet_nodeid) {
                nsp_process_short(&decnet_rx);
            }
        }
    }
    if(nsp_link.state==PORT_NSP_CONNECTED) {
        int retrans=port_nsp_tx_tick(&nsp_transport,now);
        if(retrans==1&&nsp_replay_len)
            (void)nsp_send_segment(nsp_replay,nsp_replay_len);
        if(retrans==-1) {
            nsp_replay_len=0;
            if(nsp_mirror_waiting){++nsp_mirror_timeouts;nsp_mirror_waiting=0;}
        }
    }
    if(nsp_server_enabled&&nsp_server.active) {
        int tx=port_mirror_server_tick(&nsp_server,now);
        if(tx==1&&nsp_server.replay_len)
            (void)nsp_send_nodes(nsp_server.local_node,nsp_server.peer_node,
                                  nsp_server.replay,nsp_server.replay_len);
    }
    int retry=port_nsp_link_tick(&nsp_link,now);
    if(retry==1)(void)nsp_send_probe(PORT_NSP_RCI);
    if((uint32_t)(now-decnet_hello_last_tick)<PORT_DECNET_HELLO_INTERVAL_TICKS)return;
    decnet_hello_last_tick=now;
    decnet_last_status=decnet_hello_send();
}
struct port_dcl_guest_timer {
    uint32_t start, count;
    unsigned efn, cancels, sets, waits, unwinds, rundown, aborts;
};
static uint32_t dcl_guest_expression(void *ctx,const char *expression,uint32_t *value){
    (void)ctx;
    if(eq(expression,"7+2")){*value=9u;return 1u;}
    return 2u;
}
static void dcl_guest_unstack(void *ctx){((struct port_dcl_guest_timer*)ctx)->unwinds++;}
static void dcl_guest_rundown(void *ctx){((struct port_dcl_guest_timer*)ctx)->rundown++;}
static void dcl_guest_abort(void *ctx){((struct port_dcl_guest_timer*)ctx)->aborts++;}
static int dcl_guest_cancel(void *ctx){
    ((struct port_dcl_guest_timer*)ctx)->cancels++;
    return 1;
}
static int dcl_guest_set(void *ctx,unsigned efn,uint64_t count100ns){
    struct port_dcl_guest_timer *t=ctx;
    if(efn!=31u||count100ns>1000000u)return 0;
    t->start=arch_ticks();
    t->count=((uint32_t)count100ns+99999u)/100000u; /* PIT 100 Hz */
    t->efn=efn;t->sets++;
    return 1;
}
static int dcl_guest_wait(void *ctx,unsigned efn){
    struct port_dcl_guest_timer *t=ctx;
    if(t->efn!=efn)return 0;
    t->waits++;
    while((uint32_t)(arch_ticks()-t->start)<t->count){
        if((uint32_t)(arch_ticks()-t->start)>20u)return 0;
        __asm__ volatile("sti; hlt" ::: "memory");
    }
    return 1;
}
static const struct port_dcl_ops dcl_guest_ops={
    .binary_expression=dcl_guest_expression,.unstack=dcl_guest_unstack,
    .rundown=dcl_guest_rundown,.abort_process=dcl_guest_abort,
    .cancel_timers=dcl_guest_cancel,.set_timer=dcl_guest_set,
    .wait_flag=dcl_guest_wait
};
static int dcl_exit_guest_test(void){
    struct port_dcl_guest_timer f={0};
    struct port_dcl_process p={.last_status=1u,.indirect_depth=1u,.interactive=1u};
    uint32_t result=0;
    int a=port_dcl_exit(&p,"7+2",&dcl_guest_ops,&f,&result);
    int good=a==PORT_DCL_EXIT_RETURN && p.indirect_depth==0u&&
             result==9u&&p.last_status==9u&&f.unwinds==1u;
    a=port_dcl_exit(&p,"",&dcl_guest_ops,&f,&result);
    good=good&&a==PORT_DCL_EXIT_RETURN && f.rundown==1u &&
         result==(9u|PORT_DCL_INHIB_MSG)&&p.command_flags==PORT_DCL_COMMAND;
    p=(struct port_dcl_process){.last_status=1u,.interactive=0u};
    a=port_dcl_exit(&p,"",&dcl_guest_ops,&f,&result);
    return good&&a==PORT_DCL_EXIT_LOGOUT&&f.aborts==1u&&p.exiting==1u;
}
struct port_dcl_status_guest_calls {unsigned disable,flush,reset,start;};
static void dcl_status_guest_disable(void *ctx){((struct port_dcl_status_guest_calls*)ctx)->disable++;}
static void dcl_status_guest_flush(void *ctx){((struct port_dcl_status_guest_calls*)ctx)->flush++;}
static void dcl_status_guest_reset(void *ctx){((struct port_dcl_status_guest_calls*)ctx)->reset++;}
static void dcl_status_guest_start(void *ctx){((struct port_dcl_status_guest_calls*)ctx)->start++;}
static int dcl_status_guest_test(void){
    struct port_dcl_status_process p={.last_status=1u,.on_level=2u};
    struct port_dcl_status_work w={0};
    struct port_dcl_status_guest_calls c={0};
    const struct port_dcl_status_ops ops={
        .disable_control_ast=dcl_status_guest_disable,
        .flush=dcl_status_guest_flush,.on_reset=dcl_status_guest_reset,
        .command_start=dcl_status_guest_start
    };
    int good=port_dcl_set_status(&p,&w,2u,&ops,&c)==PORT_DCL_STATUS_ON_COMMAND;
    good=good&&eq(w.command,"EXIT")&&p.last_status==2u&&
        p.severity==2u&&eq(p.status_text,"%X00000002")&&
        (p.flags&PORT_DCL_STATUS_ONEXIT)&&c.start==1u&&c.flush==1u;
    good=good&&port_dcl_set_status(&p,&w,4u,&ops,&c)==0&&
        !(p.flags&PORT_DCL_STATUS_ONEXIT)&&c.start==1u;
    p.on_error="GOTO HANDLER";
    good=good&&port_dcl_set_status(&p,&w,2u,&ops,&c)==PORT_DCL_STATUS_ON_COMMAND&&
        eq(w.command,"GOTO HANDLER")&&c.reset==2u&&c.start==2u;
    p.flags=PORT_DCL_STATUS_YLEVEL;
    good=good&&port_dcl_set_status(&p,&w,4u,&ops,&c)==0&&
        p.last_status==2u&&p.severity==4u;
    return good;
}
struct port_dcl_if_guest_calls {unsigned evaluated,marked,started;const char *command;};
static uint32_t dcl_if_guest_expr(void *ctx,const char *expr,uint32_t *value){
    struct port_dcl_if_guest_calls *calls=ctx;
    calls->evaluated++;
    if(eq(expr,"1")){*value=1u;return 1u;}
    if(eq(expr,"0")){*value=0u;return 1u;}
    return 0x12u;
}
static void dcl_if_guest_mark(void *ctx){((struct port_dcl_if_guest_calls*)ctx)->marked++;}
static int dcl_if_guest_start(void *ctx,const char *command){
    struct port_dcl_if_guest_calls *calls=ctx;
    calls->started++;calls->command=command;return 1;
}
static int dcl_if_guest_test(void){
    struct port_dcl_if_guest_calls calls={0};
    const struct port_dcl_if_ops ops={
        .normal_status=1u,.invalid_then_status=0x12u,
        .binary_expression=dcl_if_guest_expr,.mark=dcl_if_guest_mark,
        .command_start=dcl_if_guest_start
    };
    uint32_t flags=0u,result=0u;
    int valid=port_dcl_if("1","THEN","SHOW TIME",&flags,&ops,&calls,&result)==2;
    valid=valid&&(flags&PORT_DCL_IF_NOSTAT)&&result==1u&&
          calls.started==1u&&eq(calls.command,"SHOW TIME");
    valid=valid&&port_dcl_if("0","THEN","EXIT",&flags,&ops,&calls,&result)==1&&
          calls.started==1u;
    valid=valid&&port_dcl_if("1","WHEN","EXIT",&flags,&ops,&calls,&result)==0&&
          result==0x12u&&calls.marked==1u&&calls.started==1u;
    valid=valid&&port_dcl_if("bad","THEN","EXIT",&flags,&ops,&calls,&result)==0&&
          result==0x12u&&calls.marked==1u;
    return valid;
}
static int dcl_wait_guest_test(void){
    struct port_dcl_guest_timer t={0};
    uint32_t before=arch_ticks();
    int good=port_dcl_wait("00:00:00.02",&dcl_guest_ops,&t);
    uint32_t elapsed=(uint32_t)(arch_ticks()-before);
    return good && t.cancels==1u&&t.sets==1u&&t.waits==1u &&
           t.efn==31u&&t.count==2u&&elapsed>=2u&&elapsed<=20u;
}
static int uaf_guest_read(void *context,uint32_t lba,uint16_t dst[256]) {
    (void)context;
    return port_ata_x86_read28(lba,dst);
}
struct port_lgi_guest_context {
    char username[16];
    char terminal[24];
};
static int guest_terminal(void *context,char *value,unsigned cap){
    (void)context;
    static const char name[]="TTA0:";
    if(cap<sizeof name)return -1;
    for(unsigned i=0;i<sizeof name;i++)value[i]=name[i];
    return sizeof name-1u;
}
static int guest_user_name(void *context,const char *value){
    struct port_lgi_guest_context *guest=context;
    unsigned i=0;
    for(;value[i] && i<sizeof guest->username-1u;i++)
        guest->username[i]=value[i];
    guest->username[i]=0;
    return 1;
}
static int guest_term_name(void *context,const char *value){
    struct port_lgi_guest_context *guest=context;
    unsigned i=0;
    for(;value[i]&&i<sizeof guest->terminal-1u;i++)
        guest->terminal[i]=value[i];
    guest->terminal[i]=0;
    return 1;
}
static int guest_logical_lookup(void *context,const char *name,
                                uint8_t *out,unsigned cap){
    (void)context;(void)name;(void)out;(void)cap;
    return -1; /* No SYS$ANNOUNCE or SYS$WELCOME files installed. */
}
static int login_interact_guest_test(const char *expected_user) {
    struct port_lgi_guest_context owner={0};
    struct vms_lgi_uart uart;
    struct vms_lgi_fileio fileio;
    struct port_uaf_disk disk;
    struct port_uaf_service auth;
    struct vms_lgi_interact session;
    const struct vms_lgi_interact_ops ops={
        .terminal_name=guest_terminal,.set_username=guest_user_name,
        .set_termname=guest_term_name,.translate=guest_logical_lookup
    };
    vms_lgi_uart_init(&uart,arch_ticks);
    vms_lgi_fileio_init(&fileio,vms_lgi_uart_ops(),&uart);
    port_uaf_disk_init(&disk,uaf_guest_read,0,3u);
    auth=port_uaf_disk_service(&disk);
    vms_lgi_interact_init(&session,&fileio,&auth,&ops,&owner);
    int status=vms_lgi_init_interactive(&session);
    int valid=status==VMS_LOGIN_OK && session.logged_in &&
              session.user_present && !session.auto_selected &&
              eq(owner.username,expected_user) &&
              uart.input_count==2u &&
              owner.terminal[0]=='T'&&owner.terminal[1]=='T';
    if(valid && vms_lgi_announce(&session,"3.7")!=VMS_LOGIN_OK)valid=0;
    if(vms_lgi_close_input(&fileio)!=VMS_LGI_OK ||
       vms_lgi_close_output(&fileio)!=VMS_LGI_OK)valid=0;
    return valid && !fileio.input_open && !fileio.output_open;
}
static int login_fileio_guest_test(void) {
    struct vms_lgi_uart uart;
    struct vms_lgi_fileio io;
    struct port_uaf_disk disk;
    struct port_uaf_service auth;
    struct port_uaf_record record;
    uint8_t username[24], password[24];
    static const uint8_t announcement[]="FILEIO OUTPUT> ";
    int good=1;
    vms_lgi_uart_init(&uart,arch_ticks);
    vms_lgi_fileio_init(&io,vms_lgi_uart_ops(),&uart);
    if(vms_lgi_open_output(&io)!=VMS_LGI_OK ||
       vms_lgi_open_input(&io)!=VMS_LGI_OK) return 0;
    if(!io.input_shared || uart.length[0]!=uart.length[1] ||
       uart.length[2]!=uart.length[3] ||
       !uart.length[0] || !uart.length[2]) good=0;
    uint32_t sent_before=port_serial_x86_sent();
    (void)vms_lgi_write_output(&io,announcement,sizeof announcement-1u);
    if(io.last_status!=VMS_LGI_OK || port_serial_x86_sent()<=sent_before)
        good=0;
    io.input_prompt="FILEIO INPUT> ";
    io.input_prompt_length=14u;
    if(vms_lgi_get_input(&io,username,sizeof username)!=VMS_LGI_OK ||
       io.last_input_length!=7u)good=0;
    io.input_prompt="FILEIO PASSWORD> ";
    io.input_prompt_length=17u;
    io.input_flags|=VMS_LGI_READ_QUIET;
    if(vms_lgi_get_input(&io,password,sizeof password)!=VMS_LGI_OK ||
       io.last_input_length!=8u)good=0;
    port_uaf_disk_init(&disk,uaf_guest_read,0,3u);
    auth=port_uaf_disk_service(&disk);
    if(good && port_uaf_validate(&auth,username,7u,password,8u,
                                 &record,sizeof record)!=PORT_UAF_NORMAL)
        good=0;
    for(unsigned i=0;i<sizeof username;i++)username[i]=0;
    for(unsigned i=0;i<sizeof password;i++)password[i]=0;
    if(vms_lgi_close_input(&io)!=VMS_LGI_OK ||
       vms_lgi_close_output(&io)!=VMS_LGI_OK)good=0;
    return good && uart.input_count==2u &&
           uart.output_count==sizeof announcement-1u &&
           !io.input_open && !io.output_open && !uart.timer_active &&
           !uart.canceled;
}

/* Live COM1 adapter for DCL/SRC/INQUIRE.MAR. The other DCL images still
 * require source-equivalent implementations and are not marked complete. */
struct dcl_symbol {
    char name[PORT_DCL_INQUIRE_SYMBOL_MAX+1];
    char value[PORT_DCL_INQUIRE_TEXT_MAX+1];
    unsigned global;
};
struct dcl_console {
    struct dcl_symbol slots[12];
    unsigned count;
    unsigned indirect_suppressed, control_ast_disabled;
};
static struct dcl_console dcl_console;
static struct port_dcl_initial dcl_initial;
static int dcl_readline(const char *prompt,char *out,unsigned cap){
    if(!out||cap<2u)return -1;
    if(prompt)puts(prompt);
    unsigned used=0;
    for(;;){
        char c=getkey_mode(1);
        if(c=='\n'){putc('\n');out[used]=0;return (int)used;}
        if(c=='\b'){
            if(used){used--;putc('\b');}
        }else if(c>=' '&&c<='~'&&used+1u<cap){
            out[used++]=c;putc(c);
        }
    }
}
static int dcl_io_input(void *ctx,const char *prompt,char *buf,unsigned cap){
    (void)ctx;return dcl_readline(prompt,buf,cap);
}
static int dcl_io_store(void *ctx,const char *name,const char *value,unsigned global){
    struct dcl_console *c=ctx;
    unsigned pos=c->count;
    for(unsigned i=0;i<c->count;i++){
        if(c->slots[i].global==global&&eq(c->slots[i].name,name)){
            pos=i;break;
        }
    }
    if(pos==12u)return 0;
    unsigned i=0;
    for(;name[i]&&i<sizeof c->slots[pos].name-1;i++)c->slots[pos].name[i]=name[i];
    c->slots[pos].name[i]=0;
    for(i=0;value[i]&&i<sizeof c->slots[pos].value-1;i++)c->slots[pos].value[i]=value[i];
    c->slots[pos].value[i]=0;
    c->slots[pos].global=global;
    if(pos==c->count)c->count++;
    return 1;
}
static void dcl_no_indirect(void *ctx){((struct dcl_console*)ctx)->indirect_suppressed++;}
static void dcl_no_ast(void *ctx){((struct dcl_console*)ctx)->control_ast_disabled++;}
static const struct port_dcl_inquire_ops dcl_live_ops={
    .input=dcl_io_input,.store=dcl_io_store,
    .disable_indirect=dcl_no_indirect,.disable_control_ast=dcl_no_ast
};
static int dcl_inquire_guest_input(void *ctx,const char *prompt,char *answer,unsigned cap){
    struct dcl_console *c=ctx;
    if(!eq(prompt,"Enter value: ")||cap<14u)return -1;
    const char a[]="\"First \"\"answer\"\"\"";
    unsigned i=0;while(a[i]){answer[i]=a[i];i++;}answer[i]=0;
    c->indirect_suppressed++;
    return (int)i;
}
static int dcl_inquire_guest_test(void){
    struct dcl_console c={0};
    const struct port_dcl_inquire_ops ops={
        .input=dcl_inquire_guest_input,.store=dcl_io_store,
        .disable_indirect=dcl_no_indirect,.disable_control_ast=dcl_no_ast
    };
    struct port_dcl_inquire_request q;
    int ok=port_dcl_inquire_parse("/NOPUNCTUATION QUESTION",&q)==1&&
           eq(q.prompt,"QUESTION")&&q.punctuation==0;
    ok=ok&&port_dcl_inquire("/GLOBAL answer \"Enter value\"",&ops,&c)==1&&
       c.count==1u&&c.slots[0].global==1u&&
       eq(c.slots[0].name,"ANSWER")&&
       eq(c.slots[0].value,"First \"answer\"")&&
       c.control_ast_disabled==1u&&c.indirect_suppressed==2u;
    ok=ok&&port_dcl_inquire_parse("/INVALID answer",&q)==0&&
       port_dcl_inquire_parse("9BAD",&q)==0;
    return ok;
}
static int dcl_case_eq(const char *a,const char *b){
    while(*a&&*b){
        char x=*a++,y=*b++;
        if(x>='a'&&x<='z')x=(char)(x-'a'+'A');
        if(y>='a'&&y<='z')y=(char)(y-'a'+'A');
        if(x!=y)return 0;
    }
    return !*a&&!*b;
}
static void dcl_show_symbol(const char *name){
    while(*name==' '||*name=='\t')name++;
    for(unsigned i=0;i<dcl_console.count;i++){
        if(dcl_case_eq(name,dcl_console.slots[i].name)){
            puts(dcl_console.slots[i].name);puts(" = \"");
            puts(dcl_console.slots[i].value);puts("\"\n");return;
        }
    }
    char text[PORT_DCL_INITIAL_MAX_VALUE+1];
    if(port_dcl_initial_symbol(&dcl_initial,name,2u,text,sizeof text)){
        puts(name);puts(" = \"" );puts(text);puts("\"\n");return;
    }
    puts("%DCL-W-UNDSYM, undefined symbol\n");
}
static void dcl_on_status_reset(void *ctx){
    port_dcl_on_reset((struct port_dcl_on_process*)ctx);
}
static int dcl_on_guest_test(void){
    struct port_dcl_on_process p;
    port_dcl_on_init(&p,1u,0u);
    int ok=port_dcl_on(" ERROR THEN EXIT",&p)==PORT_DCL_ON_LEVEL;
    p.indirect_depth=1u;
    ok=ok&&port_dcl_on(" ERROR THEN SHOW STATUS",&p)==PORT_DCL_ON_NORMAL&&
        p.on_level==2u&&eq(p.on_error,"SHOW STATUS");
    ok=ok&&port_dcl_on(" CONTROL_Y THEN CONTINUE",&p)==PORT_DCL_ON_NORMAL&&
        p.control_enabled && eq(p.on_control_y,"CONTINUE");
    struct port_dcl_status_process status={.on_level=(uint8_t)p.on_level,
                                           .on_error=p.on_error};
    struct port_dcl_status_work work={0};
    const struct port_dcl_status_ops ops={.on_reset=dcl_on_status_reset};
    ok=ok&&port_dcl_set_status(&status,&work,2u,&ops,&p)==
           PORT_DCL_STATUS_ON_COMMAND&&eq(work.command,"SHOW STATUS")&&
           p.on_level==2u&&!p.on_error[0];
    port_dcl_on_ctly_reset(&p);
    ok=ok&&!p.on_control_y[0]&&!p.control_enabled&&
        port_dcl_on(" ERROR ELSE EXIT",&p)==PORT_DCL_ON_BADCOMMAND;
    return ok;
}
static int dcl_descr_guest_test(void){
    /* DCLDEF.MDL PTR descriptor bitfield layout. */
    const uint32_t tokens[]={
        0x87600534u,0x43200001u
    };
    struct port_dcl_descr_state state={tokens,tokens+2,"GOTO HERE",9u};
    struct port_dcl_descr_result item;
    int ok=port_dcl_getdval(&state,&item)&&
        item.value==0x34u&&*item.item=='H'&&
        item.flags==6u&&item.term==7u&&item.type==8u;
    ok=ok&&port_dcl_getdval(&state,&item)&&item.value==1u&&
        *item.item=='G'&&item.flags==2u&&item.term==3u&&item.type==4u;
    return ok&&!port_dcl_getdval(&state,&item);
}
static unsigned dcl_goto_guest_seeks;
static uint8_t dcl_goto_guest_rfa[6];
static int dcl_goto_guest_seek(void *ctx,const uint8_t rfa[6]){
    (void)ctx;
    dcl_goto_guest_seeks++;
    for(unsigned i=0;i<6;i++)dcl_goto_guest_rfa[i]=rfa[i];
    return 1;
}
static int dcl_goto_guest_test(void){
    struct port_dcl_goto_state state;
    const struct port_dcl_goto_ops ops={dcl_goto_guest_seek};
    const uint8_t rfa[]={0x20u,0x10u,0u,0u,0u,1u};
    dcl_goto_guest_seeks=0;
    port_dcl_goto_init(&state);
    int ok=port_dcl_goto(&state," MISSING",1,&ops,0)==PORT_DCL_GOTO_NORMAL;
    ok=ok&&port_dcl_goto_skipping(&state)&&state.no_status;
    ok=ok&&port_dcl_goto_label_record(&state,"OTHER",rfa)==0;
    ok=ok&&port_dcl_goto_skipping(&state);
    ok=ok&&port_dcl_goto_label_record(&state,"MISSING",rfa)==PORT_DCL_GOTO_NORMAL;
    ok=ok&&!port_dcl_goto_skipping(&state);
    ok=ok&&port_dcl_goto(&state," OTHER",1,&ops,0)==PORT_DCL_GOTO_NORMAL;
    ok=ok&&dcl_goto_guest_seeks==1u&&dcl_goto_guest_rfa[0]==0x20u;
    ok=ok&&port_dcl_goto(&state," BAD LABEL",1,&ops,0)==PORT_DCL_GOTO_BADLABEL;
    port_dcl_dealgoto(&state);
    return ok&&!state.pending;
}
static int dcl_images_guest_test(void){
    for(unsigned image=0x80u;image<0x80u+PORT_DCL_IMAGE_COUNT;image++){
        const char *name=port_dcl_image_name(image);
        if(!name||port_dcl_image_lookup(name)!=image)return 0;
    }
    return port_dcl_image_lookup("inquire")==PORT_DCL_IMAGE_INQUIRE&&
           port_dcl_image_lookup("ON")==PORT_DCL_IMAGE_ON&&
           port_dcl_image_lookup("nonsense")==0u;
}
static void dcl_live_rundown(void *ctx){
    *(struct dcl_console*)ctx=(struct dcl_console){0};
}
static const struct port_dcl_ops dcl_live_exit_ops={
    .rundown=dcl_live_rundown
};

/* DCL$INPUT receives level-zero records from COM1. This laboratory
   command file is stored on IDE until the original RMS reader is ready. */
struct dcl_record_ctx {
    char path[PORT_DCL_INDIRECT_LEVELS+1][32];
    unsigned record_index[PORT_DCL_INDIRECT_LEVELS+1];
    struct port_dcl_process *dcl;
};
static int dcl_procedure_get(unsigned at,char *dst,unsigned cap){
    uint16_t words[256];
    if(port_ata_x86_read28(8u,words))return PORT_DCL_INPUT_ERROR;
    const uint8_t *bytes=(const uint8_t *)(const void *)words;
    const char signature[]="VMSDCL1";
    for(unsigned i=0;i<7u;i++)
        if(bytes[i]!=(uint8_t)signature[i])return PORT_DCL_INPUT_ERROR;
    if(at>=bytes[8])return PORT_DCL_INPUT_EOF;
    unsigned off=9u;
    for(unsigned i=0;i<=at;i++){
        if(off>=512u)return PORT_DCL_INPUT_ERROR;
        unsigned len=bytes[off++];
        if(off+len>512u)return PORT_DCL_INPUT_ERROR;
        if(i==at){
            if(len>=cap)return PORT_DCL_INPUT_SPACE;
            for(unsigned j=0;j<len;j++)dst[j]=(char)bytes[off+j];
            dst[len]=0;
            return (int)len;
        }
        off+=len;
    }
    return PORT_DCL_INPUT_EOF;
}
static int dcl_record_get(void *context,unsigned depth,const char *prompt,
                          char *line,unsigned cap){
    struct dcl_record_ctx *io=context;
    if(!depth)return dcl_readline(prompt,line,cap);
    if(depth>PORT_DCL_INDIRECT_LEVELS||!io->path[depth][0])return PORT_DCL_INPUT_BADFILE;
    return dcl_procedure_get(io->record_index[depth]++,line,cap);
}
static int dcl_record_push(void *context,unsigned depth,const char *path){
    struct dcl_record_ctx *io=context;
    if(depth>PORT_DCL_INDIRECT_LEVELS||!dcl_case_eq(path,"LAB.COM"))return 0;
    const char name[]="LAB.COM";
    unsigned j=0;
    for(;name[j];j++)io->path[depth][j]=name[j];
    io->path[depth][j]=0;
    io->record_index[depth]=0;
    return 1;
}
static void dcl_record_pop(void *context,unsigned depth){
    struct dcl_record_ctx *io=context;
    if(depth<=PORT_DCL_INDIRECT_LEVELS){
        io->path[depth][0]=0;
        io->record_index[depth]=0;
    }
}
static int dcl_record_symbol(void *context,const char *name,char *out,unsigned cap){
    (void)context;
    for(unsigned i=0;i<dcl_console.count;i++){
        struct dcl_symbol *slot=&dcl_console.slots[i];
        if(dcl_case_eq(slot->name,name)){
            unsigned j=0;
            while(slot->value[j]&&j+1u<cap){out[j]=slot->value[j];j++;}
            if(slot->value[j])return 0;
            out[j]=0;return 1;
        }
    }
    return port_dcl_initial_symbol(&dcl_initial,name,2u,out,cap);
}
static void dcl_record_verify(void *context,const char *line){
    (void)context;puts(line);putc('\n');
}
static void dcl_record_error(void *context,int code){
    (void)context;
    if(code==PORT_DCL_INPUT_SYMBOL)puts("%DCL-W-UNDSYM, undefined symbol\n");
    else if(code==PORT_DCL_INPUT_BADFILE)puts("%DCL-E-OPENIN, command file missing\n");
    else puts("%DCL-E-READERR, command record input failed\n");
}
static void dcl_record_status(void *context,int code){
    struct dcl_record_ctx *io=context;
    if(io->dcl)io->dcl->last_status=code<0?2u:1u;
}
static struct vms_ast_process dcl_control_ast;
static void dcl_disable_control_y(void *context){
    (void)context;
    (void)vms_ast_set_enable(&dcl_control_ast,VMS_MODE_SUPER,0);
}
static void dcl_enable_control_y(void *context){
    (void)context;
    (void)vms_ast_set_enable(&dcl_control_ast,VMS_MODE_SUPER,1);
}
static const struct port_dcl_readrec_ops dcl_record_ops={
    .get=dcl_record_get,.push=dcl_record_push,.pop=dcl_record_pop,
    .disable_control_y=dcl_disable_control_y,
    .enable_control_y=dcl_enable_control_y,
    .symbol=dcl_record_symbol,.verify=dcl_record_verify,
    .error=dcl_record_error,.set_status=dcl_record_status
};
static int dcl_readrec_guest_test(void){
    struct dcl_record_ctx io={0};
    struct port_dcl_readrec state;
    char out[PORT_DCL_RECORD_SIZE];
    port_dcl_readrec_init(&state,0);
    int ok=port_dcl_special(&state,"@LAB.COM",&dcl_record_ops,&io,
                            out,sizeof out)==PORT_DCL_INPUT_INDIRECT;
    ok=ok&&state.depth==1u;
    ok=ok&&port_dcl_input(&state,&dcl_record_ops,&io)==PORT_DCL_INPUT_OK;
    ok=ok&&dcl_case_eq(state.line,"$ SHOW SYMBOL DEST");
    ok=ok&&port_dcl_input(&state,&dcl_record_ops,&io)==PORT_DCL_INPUT_OK;
    ok=ok&&dcl_case_eq(state.line,"$ SHOW SYMBOL DEST");
    ok=ok&&io.record_index[1]==3u;
    port_dcl_readrec_init(&state,0);
    const char text[]="'NOREADREC'";
    for(unsigned i=0;i<sizeof text;i++)state.line[i]=text[i];
    ok=ok&&port_dcl_expand(&state,&dcl_record_ops,&io)==PORT_DCL_INPUT_SYMBOL;
    return ok;
}
static int dcl_initial_guest_reset_oob(void *context,uint32_t mask){
    (void)context;
    return vms_ast_set_enable(&dcl_control_ast,VMS_MODE_SUPER,
                (mask&PORT_DCL_CTRLY_MASK)!=0)>=VMS_AST_OK;
}
static const struct port_dcl_initial_ops dcl_initial_guest_ops={
    .reset_oob=dcl_initial_guest_reset_oob
};
static int dcl_initial_guest_test(void){
    struct port_dcl_initial_ppd ppd={.input_channel=4u,.input_isi=1u,
        .output_isi=1u,.input_ifi=1u,.output_ifi=1u,
        .table_structure_level=5u};
    struct port_dcl_initial *p=&dcl_initial;
    vms_ast_init(&dcl_control_ast,0x370066u);
    int ok=port_dcl_initial_start(p,&ppd,&dcl_initial_guest_ops,0)==
           PORT_DCL_INITIAL_OK;
    char out[48];
    ok=ok&&p->on_level==0x208u&&p->frame_capacity==9u&&
        p->fab_length==80u&&p->nam_length==96u&&
        p->input.prompt&&p->input.isi==1u&&
        port_dcl_initial_symbol(p,"$STATUS",1u,out,sizeof out)&&
        dcl_case_eq(out,"1")&&p->outofband_mask==PORT_DCL_CTRLY_MASK;
    port_dcl_initial_set_status(p,6u);
    ok=ok&&port_dcl_initial_symbol(p,"$SEVERITY",1u,out,sizeof out)&&
        dcl_case_eq(out,"3");
    return ok;
}
/* Original COMMAND.MAR parses input before dispatching DCL image services.
   The table entries here cover commands backed by native source modules. */
static const struct port_dcl_command_rule dcl_guest_commands[]={
    {.name="INQUIRE",.image=PORT_DCL_IMAGE_INQUIRE,.flags=PORT_DCL_CMD_INTERNAL,
     .min_params=1,.max_params=2},
    {.name="ON",.image=PORT_DCL_IMAGE_ON,.flags=PORT_DCL_CMD_INTERNAL,
     .min_params=3,.max_params=5},
    {.name="GOTO",.image=PORT_DCL_IMAGE_GOTO,.flags=PORT_DCL_CMD_INTERNAL,
     .min_params=1,.max_params=1},
    {.name="SHOW",.image=PORT_DCL_IMAGE_SHOWSYMBL,.flags=PORT_DCL_CMD_INTERNAL,
     .min_params=2,.max_params=2},
    {.name="SET",.image=PORT_DCL_IMAGE_SETDEFALT,.flags=PORT_DCL_CMD_INTERNAL,
     .min_params=1,.max_params=2},
    {.name="EXIT",.image=PORT_DCL_IMAGE_EXIT,.flags=PORT_DCL_CMD_IMMEDIATE,
     .min_params=0,.max_params=1},
    {.name="LOGOUT",.image=PORT_DCL_IMAGE_LOGOUT,.flags=PORT_DCL_CMD_IMMEDIATE,
     .min_params=0,.max_params=0},
    {.name="EOD",.image=PORT_DCL_IMAGE_EOD,.flags=PORT_DCL_CMD_IMMEDIATE,
     .min_params=0,.max_params=0},
    {.name="DECK",.image=PORT_DCL_IMAGE_DECK,.flags=PORT_DCL_CMD_IMMEDIATE,
     .min_params=0,.max_params=0}
};
struct dcl_command_console {
    struct port_dcl_process *dcl;
    struct port_dcl_on_process *on;
    struct port_dcl_readrec *reader;
    struct dcl_record_ctx *records;
    struct port_dcl_goto_state *goto_state;
    unsigned logged_out;
};
static struct port_dcl_command dcl_command_state;
static struct dcl_command_console dcl_command_ctx;
static struct port_dcl_goto_state dcl_goto_levels[PORT_DCL_INDIRECT_LEVELS+1];
static const struct port_dcl_command_rule *dcl_find_command(void *v,const char *name){
    (void)v;
    for(unsigned i=0;i<sizeof dcl_guest_commands/sizeof dcl_guest_commands[0];i++)
        if(dcl_case_eq(name,dcl_guest_commands[i].name))return &dcl_guest_commands[i];
    return 0;
}
static int dcl_assign_command(void *ctx,const char *name,const char *source,
                              unsigned global,unsigned numeric,unsigned substring){
    (void)ctx;
    if(substring)return 0;
    char result[PORT_DCL_INQUIRE_TEXT_MAX+1];
    unsigned at=0;
    const char *p=source;
    while(*p==' '||*p=='\t')p++;
    if(numeric){
        unsigned total=0,sign=1,saw=0;
        while(*p){
            if(*p=='+'||*p=='-'){sign=*p=='+'?1u:0u;p++;continue;}
            if(*p<'0'||*p>'9')return 0;
            unsigned v=0;
            do{v=v*10u+(unsigned)(*p++-'0');saw=1;}while(*p>='0'&&*p<='9');
            if(sign)total+=v;else total-=v;
            while(*p==' ')p++;
            if(*p&&*p!='+'&&*p!='-')return 0;
        }
        if(!saw)return 0;
        char reverse[11];unsigned n=0;
        do{reverse[n++]=(char)('0'+(total%10u));total/=10u;}while(total);
        if(n>=sizeof result)return 0;
        for(unsigned i=0;i<n;i++)result[i]=reverse[n-i-1u];
        result[n]=0;
    }else{
        if(*p=='"'){
            p++;
            while(*p&&at+1u<sizeof result){
                if(*p=='"'){
                    if(p[1]=='"'){result[at++]='"';p+=2;continue;}
                    p++;break;
                }
                result[at++]=*p++;
            }
            while(*p==' ')p++;
            if(*p)return 0;
        }else{
            while(*p&&at+1u<sizeof result)result[at++]=*p++;
            while(at&&result[at-1u]==' ')at--;
        }
        result[at]=0;
    }
    return dcl_io_store(&dcl_console,name,result,global);
}
static int dcl_cmd_open(void *v,const char *name){
    struct dcl_command_console *ctx=v;
    char token[PORT_DCL_RECORD_SIZE],replacement[PORT_DCL_RECORD_SIZE];
    unsigned i=0;
    token[i++]='@';
    while(*name&&i+1u<sizeof token)token[i++]=*name++;
    if(*name)return 0;
    token[i]=0;
    return port_dcl_special(ctx->reader,token,&dcl_record_ops,
                            ctx->records,replacement,sizeof replacement)==
                            PORT_DCL_INPUT_INDIRECT;
}
static int dcl_cmd_symbol(void *ctx,const char *name,char *value,unsigned cap){
    (void)ctx;return dcl_record_symbol(0,name,value,cap);
}
static int dcl_cmd_prompt(void *ctx,const char *name,char *out,unsigned cap){
    (void)ctx;
    char prompt[PORT_DCL_CMD_PROMPT_MAX];
    unsigned i=0;
    prompt[i++]='$';prompt[i++]='_';
    while(*name&&i+3u<sizeof prompt)prompt[i++]=*name++;
    prompt[i++]=':';prompt[i++]=' ';prompt[i]=0;
    return dcl_readline(prompt,out,cap);
}
static int dcl_goto_record_position(void *v,const uint8_t rfa[6]){
    struct dcl_command_console *p=v;
    if(!p||!p->records||!p->reader->depth)return 0;
    unsigned i=(unsigned)rfa[0]|((unsigned)rfa[1]<<8);
    p->records->record_index[p->reader->depth]=i;
    return 1;
}
static const struct port_dcl_goto_ops dcl_goto_record_ops={
    .find_rfa=dcl_goto_record_position
};
static int dcl_command_label(void *v,const char *name){
    struct dcl_command_console *p=v;
    if(!p||!p->reader->depth||!p->goto_state)return 0;
    unsigned record=p->records->record_index[p->reader->depth];
    if(!record)return 0;
    record--;
    uint8_t rfa[6]={(uint8_t)record,(uint8_t)(record>>8),0,0,0,0};
    return port_dcl_goto_label_record(p->goto_state,name,rfa)==
        PORT_DCL_GOTO_NORMAL;
}
static int dcl_command_seek_label(void *v,const char *name){
    struct dcl_command_console *p=v;
    if(!p||!p->goto_state)return 0;
    (void)dcl_command_label(v,name);
    if(port_dcl_goto_skipping(p->goto_state))return 0;
    p->reader->goto_search=0;
    return 1;
}
static void dcl_cmd_error(void *v,int status){
    (void)v;
    if(status==PORT_DCL_CMD_BADVERB)
        puts("%DCL-W-IVVERB, command verb not recognized\n");
    else if(status==PORT_DCL_CMD_MAXPARM)
        puts("%DCL-E-MAXPARM, too many parameters\n");
    else if(status==PORT_DCL_CMD_MISSING)
        puts("%DCL-E-INSFPRM, missing parameter\n");
    else if(status==PORT_DCL_CMD_NOLIST)
        puts("%DCL-E-NOLIST, list not allowed\n");
    else if(status==PORT_DCL_CMD_NOCONCAT)
        puts("%DCL-E-NOCCAT, concatenation not allowed\n");
    else if(status==PORT_DCL_CMD_INPUT)
        puts("%DCL-E-OPENIN, command file not found\n");
    else puts("%DCL-E-IVVERB, command syntax or service error\n");
}
static void dcl_cmd_status(void *ctx,uint32_t value){
    struct dcl_command_console *p=ctx;
    if(p->dcl)p->dcl->last_status=value;
    port_dcl_initial_set_status(&dcl_initial,value);
}
static int dcl_cmd_dispatch(void *ctx,const struct port_dcl_command *cmd,
                            const struct port_dcl_command_rule *rule){
    struct dcl_command_console *p=ctx;
    const struct port_dcl_command_token *a=port_dcl_cmd_parameter(cmd,0);
    const struct port_dcl_command_token *b=port_dcl_cmd_parameter(cmd,1);
    if(rule->image==PORT_DCL_IMAGE_LOGOUT||rule->image==PORT_DCL_IMAGE_EXIT){
        uint32_t status=0;
        (void)port_dcl_exit(p->dcl,"",&dcl_live_exit_ops,&dcl_console,&status);
        puts("Process logged out\n");p->logged_out=1;
        return PORT_DCL_CMD_LOGOUT;
    }
    if(rule->image==PORT_DCL_IMAGE_INQUIRE){
        const char *raw=cmd->input;
        while(*raw==' ')raw++;
        if(*raw=='$'){raw++;while(*raw==' ')raw++;}
        while(*raw&&*raw!=' '&&*raw!='/')raw++;
        int rc=port_dcl_inquire(raw,&dcl_live_ops,&dcl_console);
        if(rc!=PORT_DCL_INQUIRE_OK){
            puts(rc==PORT_DCL_INQUIRE_SYNTAX?
                 "%DCL-E-EXPSYN, invalid INQUIRE syntax\n":
                 "%DCL-E-IO, input or symbol storage failed\n");
            return PORT_DCL_CMD_SYNTAX;
        }
        return PORT_DCL_CMD_OK;
    }
    if(rule->image==PORT_DCL_IMAGE_GOTO&&a){
        int random=p->reader->depth!=0u;
        int rc=port_dcl_goto(p->goto_state,a->value,random,
                             &dcl_goto_record_ops,p);
        if(rc!=PORT_DCL_GOTO_NORMAL)return PORT_DCL_CMD_SYNTAX;
        dcl_command_state.goto_pending=port_dcl_goto_skipping(p->goto_state);
        p->reader->goto_search=dcl_command_state.goto_pending;
        dcl_command_state.no_status=1;
        return PORT_DCL_CMD_OK;
    }
    if(rule->image==PORT_DCL_IMAGE_ON){
        const char *raw=cmd->input;
        while(*raw==' ')raw++;
        if(*raw=='$'){raw++;while(*raw==' ')raw++;}
        while(*raw&&*raw!=' ')raw++;
        int rc=port_dcl_on(raw,p->on);
        if(rc==PORT_DCL_ON_LEVEL)
            puts("%DCL-E-ONLEVL, ON unavailable at direct command level\n");
        else if(rc!=PORT_DCL_ON_NORMAL)
            puts("%DCL-E-ONERR, invalid ON command\n");
        return PORT_DCL_CMD_OK;
    }
    if(rule->image==PORT_DCL_IMAGE_SHOWSYMBL&&a&&b&&
       dcl_case_eq(a->value,"SYMBOL")){
        dcl_show_symbol(b->value);
        return PORT_DCL_CMD_OK;
    }
    if(rule->image==PORT_DCL_IMAGE_SETDEFALT&&a&&b&&
       dcl_case_eq(a->value,"HOST")&&dcl_case_eq(b->value,"0")){
        puts("%DCL-E-NONET, DECnet remote terminal service not implemented\n");
        return PORT_DCL_CMD_OK;
    }
    if(rule->image==PORT_DCL_IMAGE_EOD){
        return port_dcl_cmd_eod(&dcl_command_state,"");
    }
    if(rule->image==PORT_DCL_IMAGE_DECK)return PORT_DCL_CMD_SYNTAX;
    puts("%DCL-W-IVVERB, command image not implemented\n");
    return PORT_DCL_CMD_OK;
}
static void dcl_cmd_ast(void *ctx,unsigned enabled){
    if(enabled)dcl_enable_control_y(ctx);
    else dcl_disable_control_y(ctx);
}
static const struct port_dcl_command_ops dcl_live_command_ops={
    .find_verb=dcl_find_command,.symbol=dcl_cmd_symbol,
    .assign=dcl_assign_command,.open_procedure=dcl_cmd_open,
    .label=dcl_command_label,.seek_label=dcl_command_seek_label,
    .read_parameter=dcl_cmd_prompt,.dispatch=dcl_cmd_dispatch,
    .report=dcl_cmd_error,.set_status=dcl_cmd_status,
    .control_ast=dcl_cmd_ast
};
static void dcl_interactive(const struct port_dcl_initial_ppd *ppd){
    dcl_console=(struct dcl_console){0};
    puts("DCL source commands active: INQUIRE, EXIT, LOGOUT; other images pending.\n");
    struct port_dcl_process dcl={.interactive=1,.last_status=1u};
    struct port_dcl_readrec reader;
    struct dcl_record_ctx io={0};
    struct port_dcl_on_process on;
    vms_ast_init(&dcl_control_ast,0x370066u);
    if(port_dcl_initial_start(&dcl_initial,ppd,
                              &dcl_initial_guest_ops,0)!=PORT_DCL_INITIAL_OK){
        puts("%DCL-F-INITERR, startup failed\n");return;
    }
    io.dcl=&dcl;
    port_dcl_readrec_init(&reader,0);
    port_dcl_command_init(&dcl_command_state);
    for(unsigned j=0;j<=PORT_DCL_INDIRECT_LEVELS;j++)
        port_dcl_goto_init(&dcl_goto_levels[j]);
    port_dcl_on_init(&on,1u,0u);
    dcl_command_ctx=(struct dcl_command_console){
        .dcl=&dcl,.on=&on,.reader=&reader,.records=&io,
        .goto_state=&dcl_goto_levels[0]
    };
    for(;;){
        if(!reader.depth){reader.prompt[0]='$';reader.prompt[1]=' ';reader.prompt[2]=0;}
        int rc=PORT_DCL_INPUT_OK;
        if(!port_dcl_initial_take_command(&dcl_initial,reader.line,
                                           sizeof reader.line))
            rc=port_dcl_input(&reader,&dcl_record_ops,&io);
        if(rc==PORT_DCL_INPUT_LOGOUT)return;
        if(rc!=PORT_DCL_INPUT_OK)continue;
        port_dcl_initial_set_status(&dcl_initial,dcl.last_status);
        char *cmd=reader.line;
        while(*cmd==' '||*cmd=='\t')cmd++;
        if(!*cmd||*cmd=='!')continue;
        if(*cmd=='&'){
            char token[PORT_DCL_RECORD_SIZE];
            char replacement[PORT_DCL_RECORD_SIZE];
            unsigned n=0;
            while(cmd[n]&&cmd[n]!=' '&&cmd[n]!='\t')n++;
            if(n+1u>=sizeof token)continue;
            for(unsigned i=0;i<n;i++)token[i]=cmd[i];
            token[n]=0;
            if(port_dcl_special(&reader,token,&dcl_record_ops,&io,
                                replacement,sizeof replacement)!=PORT_DCL_INPUT_OK){
                dcl_record_error(&io,PORT_DCL_INPUT_SYMBOL);
                continue;
            }
            char updated[PORT_DCL_RECORD_SIZE];
            unsigned i=0,j=0;
            while(replacement[i]&&i+1u<sizeof updated){updated[i]=replacement[i];i++;}
            while(cmd[n+j]&&i+1u<sizeof updated)updated[i++]=cmd[n+j++];
            updated[i]=0;
            for(i=0;updated[i];i++)reader.line[i]=updated[i];
            reader.line[i]=0;cmd=reader.line;
        }
        dcl.indirect_depth=(uint16_t)reader.depth;
        on.indirect_depth=reader.depth;
        dcl_command_state.depth=reader.depth;
        dcl_command_ctx.goto_state=&dcl_goto_levels[reader.depth];
        int cmd_rc=port_dcl_command_run(&dcl_command_state,cmd,
                                        &dcl_live_command_ops,&dcl_command_ctx);
        if(cmd_rc==PORT_DCL_CMD_LOGOUT||dcl_command_ctx.logged_out)return;
    }
}
static int dcl_command_guest_test(void){
    struct port_dcl_process state={.interactive=1,.last_status=1u};
    struct port_dcl_on_process on;
    struct port_dcl_readrec reader;
    struct dcl_record_ctx rec={0};
    port_dcl_on_init(&on,1u,0u);
    port_dcl_readrec_init(&reader,0);
    port_dcl_command_init(&dcl_command_state);
    port_dcl_goto_init(&dcl_goto_levels[0]);
    dcl_console=(struct dcl_console){0};
    rec.dcl=&state;
    dcl_command_ctx=(struct dcl_command_console){
        .dcl=&state,.on=&on,.reader=&reader,.records=&rec,
        .goto_state=&dcl_goto_levels[0]
    };
    int ok=port_dcl_command_run(&dcl_command_state,
        "DEST :== \"Native Guest\"",&dcl_live_command_ops,
        &dcl_command_ctx)==PORT_DCL_CMD_OK;
    char text[48];
    ok=ok&&dcl_record_symbol(&rec,"DEST",text,sizeof text)&&
        dcl_case_eq(text,"Native Guest");
    ok=ok&&port_dcl_command_run(&dcl_command_state,
        "SHOW SYMBOL DEST",&dcl_live_command_ops,
        &dcl_command_ctx)==PORT_DCL_CMD_OK;
    ok=ok&&dcl_command_state.image==PORT_DCL_IMAGE_SHOWSYMBL&&
        dcl_command_state.param_count==2u;
    ok=ok&&port_dcl_command_run(&dcl_command_state,
        "SHO :== SHOW SYMBOL",&dcl_live_command_ops,
        &dcl_command_ctx)==PORT_DCL_CMD_OK;
    ok=ok&&port_dcl_command_run(&dcl_command_state,
        "SHO DEST",&dcl_live_command_ops,
        &dcl_command_ctx)==PORT_DCL_CMD_OK;
    ok=ok&&port_dcl_command_run(&dcl_command_state,
        "SHOW SYMBOL,DEST",&dcl_live_command_ops,
        &dcl_command_ctx)==PORT_DCL_CMD_NOLIST;
    ok=ok&&port_dcl_command_run(&dcl_command_state,
        "@LAB.COM",&dcl_live_command_ops,
        &dcl_command_ctx)==PORT_DCL_CMD_SKIPPED;
    ok=ok&&reader.depth==1u;
    dcl_record_pop(&rec,reader.depth);
    return ok;
}
static void dcl_login(void){
    struct port_lgi_guest_context owner={0};
    struct vms_lgi_uart uart;
    struct vms_lgi_fileio fileio;
    struct port_uaf_disk disk;
    struct port_uaf_service auth;
    struct vms_lgi_interact session;
    const struct vms_lgi_interact_ops ops={
        .terminal_name=guest_terminal,.set_username=guest_user_name,
        .set_termname=guest_term_name,.translate=guest_logical_lookup
    };
    vms_lgi_uart_init(&uart,arch_ticks);
    vms_lgi_fileio_init(&fileio,vms_lgi_uart_ops(),&uart);
    port_uaf_disk_init(&disk,uaf_guest_read,0,3u);
    auth=port_uaf_disk_service(&disk);
    vms_lgi_interact_init(&session,&fileio,&auth,&ops,&owner);
    int rc=vms_lgi_init_interactive(&session);
    if(rc==VMS_LOGIN_OK&&session.logged_in){
        struct port_dcl_initial_ppd cli={0};
        cli.input_channel=fileio.input_channel;
        cli.input_isi=fileio.input_isi;
        cli.output_isi=fileio.output_isi;
        cli.input_ifi=fileio.input_ifi;
        cli.output_ifi=fileio.output_ifi;
        cli.input_devchar=fileio.input_characteristics;
        cli.output_devchar=fileio.output_characteristics;
        cli.table_structure_level=5u;
        for(unsigned i=0;i<16u;i++){
            cli.input_device[i]=(char)fileio.input_dvi[i];
            cli.output_device[i]=(char)fileio.output_dvi[i];
        }
        (void)vms_lgi_announce(&session,"3.7");
        (void)vms_lgi_close_input(&fileio);
        (void)vms_lgi_close_output(&fileio);
        dcl_interactive(&cli);
    }else{
        puts("%LOGIN-F-NOTVALID, login failed\n");
        (void)vms_lgi_close_input(&fileio);
        (void)vms_lgi_close_output(&fileio);
    }
}
static __attribute__((unused)) void command(const char *line) {
    if (!*line) return;
    if (eq(line,"LOGIN")){dcl_login();return;}
    if(eq(line,"TEST REMPROTO")){
        puts(port_rem_protocol_guest_selftest()?
             "PASS: REM PROTOCOL guest config, version, ECO, disconnect AST, window and RTT receive.\n":
             "FAIL: REM PROTOCOL native guest configuration.\n");
        return;
    }
    if(eq(line,"TEST DCLCOMMAND")){
        puts(dcl_command_guest_test()?
             "PASS: DCL COMMAND real guest parse, symbols, dispatch, errors and indirect open.\n":
             "FAIL: DCL COMMAND native guest command processing.\n");
        return;
    }
    if(eq(line,"TEST DCLINITIAL")){
        puts(dcl_initial_guest_test()?
             "PASS: DCL INITIAL live AST, default symbols, RMS blocks and indirect frames.\n":
             "FAIL: DCL INITIAL native process setup.\n");
        return;
    }
    if(eq(line,"TEST DCLREAD")){
        puts(dcl_readrec_guest_test()?
             "PASS: DCL READREC IDE command records, skipping, symbols, indirect levels.\n":
             "FAIL: DCL READREC guest disk and command records.\n");
        return;
    }
    if(eq(line,"TEST DCLDESCR")){
        puts(dcl_descr_guest_test()?
             "PASS: DCL descriptor value, offset, flag, term, type and cursor.\n":
             "FAIL: DCL descriptor table fields or cursor.\n");
        return;
    }
    if(eq(line,"TEST DCLGOTO")){
        puts(dcl_goto_guest_test()?
             "PASS: DCL GOTO forward label, RFA seek, status and cleanup.\n":
             "FAIL: native DCL GOTO label positioning.\n");
        return;
    }
    if(eq(line,"TEST DCLON")){
        puts(dcl_on_guest_test()?"PASS: DCL ON level, handler, CONTROL Y, status restart.\n":
                               "FAIL: DCL ON condition handler and status.\n");
        return;
    }
    if(eq(line,"TEST DCLIMAGES")){
        puts(dcl_images_guest_test()?"PASS: 43 original DCL internal image numbers.\n":
                                   "FAIL: DCL internal image lookup.\n");
        return;
    }
    if (eq(line,"TEST DCLINQUIRE")){
        puts(dcl_inquire_guest_test()?
             "PASS: DCL INQUIRE prompt, options, symbol value, AST gating.\n":
             "FAIL: DCL INQUIRE symbol and input semantics.\n");
        return;
    }
    if (eq(line, "HELP")) {
        puts("SHOW SYSTEM / CPU / A20 / DEVICES / TIMER / RINGS / SOURCES / SERIAL / QIO / CHANNEL / USERPTR / USRCHAN / USRREAD\n");
        puts("SHOW AST, SET IPL 0..9, QUEUE AST, QUEUE KAST\n");
        puts("AST ENABLE, AST DISABLE, SET RETURN MODE 0..3\n");
        puts("SHOW EXCEPTION, TEST BREAK, TEST OPCODE, TEST RING3\n");
        puts("TEST USERFAULT, TEST USERAST, TEST SCHED, SHOW SCHED\n") ;
        puts("TEST PREEMPT, TEST ISOLATION, TEST PCB, TEST QIO, TEST CHANNEL, TEST USERPTR, TEST USRCHAN, TEST USRREAD, TEST USRCANCEL, TEST USRTEAR, TEST USRCOPYIN, TEST USRDESC, CLS, HALT\n");
        puts("SHOW PCB, SHOW MEMORY, SHOW KSTACK, TEST KSTACK\n");
        puts("TEST DEMAND, SHOW VM, TEST EVENT, SHOW EVENT, TEST ASTDEL, TEST POSTEF, TEST MUTEX, TEST MUTEXCPL3, TEST HPWD, TEST VALIDATE, TEST LOGINTIME, TEST FILEIO, TEST LOGINCMD, TEST INTERACT, TEST SYSTEMLOGIN, TEST DCLEXIT, TEST DCLWAIT, TEST DCLSTATUS, TEST DCLIF, TEST DCLINQUIRE, TEST DCLON, TEST DCLIMAGES, TEST DCLDESCR, TEST DCLGOTO, TEST DCLINITIAL, TEST DCLCOMMAND, TEST REMPROTO, LOGIN\n");
        puts("TEST DISK, SHOW DISK, TEST NIC, SHOW NIC, TEST NETIO, POLL NIC, TEST DECNET, DECNET TX, POLL DECNET, TEST HELLO, HELLO TX\n");
        puts("DECNET ON, DECNET OFF, SHOW DECNET, POLL HELLO, POLL ROUTER\n");
        puts("TEST NSP, TEST SESSION, TEST DATA, TEST RELIABLE, TEST MIRROR, TEST SERVER, SHOW NSP, NSP OPEN, NSP PING, NSP CLOSE, NSP RESET, NSP POLL\n");
    } else if (eq(line, "SHOW SYSTEM")) {
        puts("Native IA-32 VAX/VMS 3.7 PORT RESEARCH milestone 29.\n");
        puts("AST queue and condition dispatcher: INDEPENDENT x86 cores.\n");
        puts("Original executive, DCL, RMS and full NSP: NOT PORTED.\n");
    } else if (eq(line, "SHOW CPU")) {
        show_cpu();
    } else if (eq(line, "SHOW RAM")) {
        puts(arch_ram_window_ready() ? "3MiB RAM window alias probe PASS.\n" : "3MiB RAM window alias probe FAIL.\n");
    } else if (eq(line, "SHOW A20")) {
        puts(arch_a20_ready() ? "A20 enabled and verified.\n" : "A20 DISABLED.\n");
    } else if (eq(line, "SHOW DEVICES")) {
        puts("Active: VGA, polled keyboard, PIC/PIT.\n");
        puts("ATA primary master (TEST DISK); NE2000 DMA RX/TX (TEST NETIO).\n");
    } else if (eq(line, "SHOW CLOCK")) {
        show_clock();
    } else if (eq(line, "SHOW AST") || eq(line, "SHOW IPL")) {
        show_ast();
    } else if (eq(line, "QUEUE AST")) {
        puts(queue_ast(VMS_AST_NORMAL,VMS_MODE_USER)==VMS_AST_OK ?
             "Queued normal user-mode AST.\n" : "AST enqueue failed.\n");
    } else if (eq(line, "QUEUE KAST")) {
        puts(queue_ast(VMS_AST_SPECIAL_KERNEL,VMS_MODE_KERNEL)==VMS_AST_OK ?
             "Queued special kernel AST.\n" : "AST enqueue failed.\n");
    } else if (eq(line, "AST ENABLE")) {
        vms_ast_set_enable(&ast_proc,VMS_MODE_USER,1);
        puts("User-mode AST enabled.\n");
    } else if (eq(line, "AST DISABLE")) {
        vms_ast_set_enable(&ast_proc,VMS_MODE_USER,0);
        puts("User-mode AST disabled.\n");
    } else if (begins(line, "SET RETURN MODE ") && line[16]>='0' &&
               line[16]<='3' && line[17]==0) {
        return_mode=(unsigned)(line[16]-'0');
        puts("Simulated return mode updated; actual x86 CPL is still 0.\n");
    } else if (begins(line, "SET IPL ") && line[8]>='0' &&
               line[8]<='9' && line[9]==0) {
        (void)vms_ast_set_ipl(&ast_proc,(unsigned)(line[8]-'0'));
        puts("Software IPL updated.\n");
    } else if (eq(line, "SHOW EXCEPTION")) {
        show_conditions();
    } else if (eq(line, "TEST BREAK")) {
        arch_probe_int3();
        puts("INT3 exception handled; resumed at next instruction.\n");
    } else if (eq(line, "TEST OPCODE")) {
        arch_probe_ud2();
        puts("UD2 exception handled; skipped known two-byte test opcode.\n");
    } else if (eq(line, "TEST RING3")) {
        uint32_t before=arch_user_exits();
        arch_run_user_test(0);
        puts(arch_user_cpl()==3 && arch_user_exits()==before+1 ?
             "PASS: CPL3 entry, syscall, kernel stack, EXIT.\n" :
             "FAIL: CPL3 probe invariants.\n");
    } else if (eq(line, "TEST USERFAULT")) {
        uint32_t before=arch_user_faults();
        arch_run_user_test(1);
        puts(arch_user_cpl()==3 && arch_user_faults()==before+1 ?
             "PASS: CPL3 write denied, #PF handled, EXIT.\n" :
             "FAIL: user page-fault probe invariants.\n");
    } else if (eq(line, "TEST INTERACT")) {
        puts(login_interact_guest_test("LABUSER") ?
            "PASS: INTERACT real COM1 LOGINOUT prompts, UAF password and welcome.\n" :
            "FAIL: LOGINOUT interactive credential processing.\n");
    } else if (eq(line, "TEST SYSTEMLOGIN")) {
        puts(login_interact_guest_test("SYSTEM") ?
            "PASS: SYSTEM authenticated via original VMS UAF record and MANAGER CRC.\n" :
            "FAIL: original VMS SYSTEM account or password rejected.\n");
    } else if (eq(line,"TEST DCLEXIT")) {
        puts(dcl_exit_guest_test() ?
            "PASS: DCL EXIT expression, indirect unstack, rundown, batch logout.\n" :
            "FAIL: native DCL EXIT status or process transitions.\n");
    } else if (eq(line,"TEST DCLIF")) {
        puts(dcl_if_guest_test() ?
            "PASS: DCL IF expression, THEN branch, NOSTAT, invalid syntax.\n" :
            "FAIL: DCL IF branch or error handling.\n");
    } else if (eq(line,"TEST DCLSTATUS")) {
        puts(dcl_status_guest_test() ?
            "PASS: DCL STATUS severity, symbols, ON ERROR, EXIT guard, YLEVEL.\n" :
            "FAIL: DCL STATUS failure dispatch or symbols.\n");
    } else if (eq(line,"TEST DCLWAIT")) {
        puts(dcl_wait_guest_test() ?
            "PASS: DCL WAIT PIT timer, event 31, elapsed guest ticks.\n" :
            "FAIL: native DCL WAIT PIT timer or event.\n");
    } else if (eq(line, "TEST LOGINCMD")) {
        struct vms_login_command parsed;
        int ok=vms_login_parse("LOGIN SYSTEM/DISK=DUA0:/CLI=DCL/COMMAND=LOGIN.COM",
                                &parsed)==VMS_LOGIN_CMD_OK &&
               parsed.has_username && parsed.has_disk && parsed.has_cli &&
               parsed.has_command_value && parsed.command_enabled &&
               parsed.username[0]=='S' && parsed.username[5]=='M' &&
               parsed.disk[0]=='D' && parsed.disk[4]==':' &&
               parsed.cli[0]=='D' && parsed.command[0]=='L';
        ok=ok && vms_login_parse("LOGIN /COMMAND",&parsed)==VMS_LOGIN_CMD_PROMPT &&
            vms_login_parse("LOGIN SYSTEM/C=FOO",&parsed)==VMS_LOGIN_CMD_AMBIGUOUS &&
            vms_login_parse("LOGIN SYSTEM/NOCOMMAND",&parsed)==VMS_LOGIN_CMD_OK &&
            !parsed.command_enabled;
        puts(ok ? "PASS: LOGINCMD username, DISK, CLI, COMMAND defaults and errors.\n" :
                  "FAIL: LOGIN command table or argument parsing.\n");
    } else if (eq(line, "TEST FILEIO")) {
        puts(login_fileio_guest_test() ?
             "PASS: FILEIO real COM1 records, UAF password, process names and close.\n" :
             "FAIL: LOGIN FILEIO serial stream or process names.\n");
    } else if (eq(line, "TEST LOGINTIME")) {
        uint64_t epoch=0,now_ticks=0;
        uint32_t day=0,hour=0;
        struct port_time_source live={port_rtc_x86_now,0};
        int ok=port_login_day_of_week(&epoch,&day,0)==PORT_LIB_NORMAL && day==3u &&
               port_login_hour_of_day(&epoch,&hour,0)==PORT_LIB_NORMAL && hour==0u;
        ok=ok && port_time_calendar_to_ticks(1970,1,1,23,0,0,&epoch) &&
            port_login_day_of_week(&epoch,&day,0)==PORT_LIB_NORMAL && day==4u &&
            port_login_hour_of_day(&epoch,&hour,0)==PORT_LIB_NORMAL && hour==23u;
        ok=ok && port_rtc_x86_now(0,&now_ticks) && now_ticks>epoch &&
            port_login_day_of_week(0,&day,&live)==PORT_LIB_NORMAL &&
            day>=1u && day<=7u &&
            port_login_hour_of_day(0,&hour,&live)==PORT_LIB_NORMAL && hour<24u;
        puts(ok ? "PASS: LIBDAYWK/LIBHOUR VMS epoch and live PC RTC.\n" :
                  "FAIL: LOGIN calendar services or PC RTC.\n");
    } else if (eq(line, "TEST VALIDATE")) {
        uint32_t errors=port_uaf_guest_selftest(uaf_guest_read,0);
        if (!errors) puts("PASS: VALIDATE indexed UAF disk, search, passwords, DEFAULT, privileges.\n");
        else { puts("FAIL: VALIDATE disk or password status=0x");
               hex32(errors); putc('\n'); }
    } else if (eq(line, "TEST HPWD")) {
        if (port_hpwd_selftest() == 0)
            puts("PASS: HPWD CRC and Purdy login hashes, salt, username, match.\n");
        else
            puts("FAIL: HPWD login hash selftest.\n");
    } else if (eq(line, "TEST MUTEX")) {
        uint32_t failures=port_mutex_selftest();
        puts(!failures ? "PASS: MUTEX read/write, pending writers, wait queues, resources, IO mutex, priority.\n" :
                         "FAIL: MUTEX executive assertions; mask=0x");
        if(failures){hex32(failures);putc('\n');}
    } else if (eq(line, "TEST MUTEXCPL3")) {
        arch_run_mutex_test();
        puts(arch_mutex_report(0)==7u && arch_mutex_report(1)==31u &&
             arch_mutex_waits()==2 && arch_mutex_wakes()==2 &&
             !arch_mutex_remaining() && !arch_mutex_held() &&
             arch_sched_done()==2 ?
             "PASS: two CPL3 tasks, contested mutex, queued wake, RWAIT/RAVAIL, release.\n" :
             "FAIL: CPL3 MUTEX wait/release/scheduler assertions.\n");
    } else if (eq(line, "TEST POSTEF")) {
        uint32_t failures=port_postef_selftest();
        puts(failures==0 ? "PASS: POSTEF PID/cluster set, AND/OR wake, common sync, backlog.\n" :
                           "FAIL: POSTEF executive assertions; mask=0x");
        if (failures) { hex32(failures); putc('\n'); }
    } else if (eq(line, "TEST ASTDEL")) {
        uint32_t failures=port_astdel_selftest();
        puts(failures==0 ? "PASS: ASTDEL process queue, KAST/IPAST, mode gating, swap/remove, wakeups.\n" :
                           "FAIL: ASTDEL native executive assertions; mask=0x");
        if (failures) { hex32(failures); putc('\n'); }
    } else if (eq(line, "TEST USERAST")) {
        vms_ast_init(&ring3_ast_proc,0x370003);
        ring3_ast_request.next=0;
        ring3_ast_request.pid=ring3_ast_proc.pid;
        ring3_ast_request.parameter=0x37a57a57;
        ring3_ast_request.callback=ast_callback;
        ring3_ast_request.context=0;
        ring3_ast_request.kind=VMS_AST_NORMAL;
        ring3_ast_request.mode=VMS_MODE_USER;
        ring3_ast_request.state=0;
        struct vms_ast_request *picked=0;
        int ok=vms_ast_enqueue(&ring3_ast_proc,&ring3_ast_request)==VMS_AST_OK;
        ok=ok && vms_ast_take(&ring3_ast_proc,VMS_MODE_USER,&picked)==VMS_AST_OK;
        if (ok && picked==&ring3_ast_request) {
            arch_run_user_ast(picked->parameter);
            ok=arch_user_ast_mode()==3 &&
               arch_user_ast_value()==picked->parameter &&
               vms_ast_complete(&ring3_ast_proc,picked)==VMS_AST_OK &&
               ring3_ast_proc.delivered==1;
        } else ok=0;
        puts(ok ? "PASS: queued AST delivered in ring3 via syscall.\n" :
                  "FAIL: ring3 AST trampoline.\n");
    } else if (eq(line, "TEST SCHED")) {
        arch_run_scheduler_test();
        puts(arch_sched_reports()==6 && arch_sched_a()==3 &&
             arch_sched_b()==3 && arch_sched_done()==2 &&
             arch_sched_switches()>=5 && arch_sched_sequence()==0x121212 ?
             "PASS: two CPL3 contexts, cooperative switches and exit.\n" :
             "FAIL: CPL3 scheduler probe invariants.\n");
    } else if (eq(line, "TEST PREEMPT")) {
        arch_run_preempt_test();
        puts(arch_sched_reports()==6 && arch_sched_a()==3 &&
             arch_sched_b()==3 && arch_sched_done()==2 &&
             arch_sched_preemptions()>=1 && arch_sched_ast_delivered()==1 &&
             arch_sched_auto_ast_reports()==1 &&
             arch_sched_auto_ast_completed()==1 &&
             arch_user_ast_mode()==3 && arch_user_ast_value()==0x0037A570u ?
             "PASS: PIT preemption, two address spaces, AST IRET trampoline.\n" :
             "FAIL: preemption/AST invariants.\n");
    } else if (eq(line, "TEST PCB")) {
        arch_run_vm_test();
        puts(arch_vm_isolated() && arch_vm_released() && arch_pcb_valid() ?
             "PASS: PCB isolation, private allocated frames, reclaim.\n" :
             "FAIL: PCB/physical allocator test.\n");
    } else if (eq(line, "TEST NIC")) {
        nic_last_result=port_ne2k_x86_probe(&nic_info);
        puts(nic_last_result==0 ?
             "PASS: ISA NE2000 reset, PROM MAC and receive-ring setup.\n" :
             "FAIL: NE2000 absent, timeout or invalid station MAC.\n");
    } else if (eq(line, "TEST HELLO")) {
        int n=port_decnet_build_endnode_hello(nic_buffer,sizeof nic_buffer,
                                              decnet_nodeid,576,15);
        int rc=n>0?port_decnet_parse_endnode_hello(nic_buffer,(unsigned)n,&decnet_peer):-1;
        puts(rc==0&&decnet_peer.nodeid==decnet_nodeid ?
             "PASS: Phase IV Endnode Hello encode/decode.\n" :
             "FAIL: Phase IV Endnode Hello validation.\n");
    } else if (eq(line, "HELLO TX")) {
        if(nic_last_result!=0)nic_last_result=port_ne2k_x86_probe(&nic_info);
        if(nic_last_result==0) {
            uint8_t station[6];port_decnet_node_mac(decnet_nodeid,station);
            nic_last_result=port_ne2k_x86_set_station(&nic_info,station);
        }
        decnet_last_status=nic_last_result==0?decnet_hello_send():-1;
        puts(decnet_last_status==0 ? "Endnode Hello multicast TX complete.\n" :
             "HELLO TX failed.\n");
    } else if (eq(line, "DECNET ON")) {
        if(nic_last_result!=0)nic_last_result=port_ne2k_x86_probe(&nic_info);
        if(nic_last_result==0) {
            uint8_t station[6];port_decnet_node_mac(decnet_nodeid,station);
            nic_last_result=port_ne2k_x86_set_station(&nic_info,station);
        }
        decnet_enabled=(nic_last_result==0);
        decnet_hello_last_tick=arch_ticks()-PORT_DECNET_HELLO_INTERVAL_TICKS;
        puts(decnet_enabled?"Periodic Phase IV Hello enabled (15 s).\n":
             "NIC unavailable; DECnet remains off.\n");
    } else if (eq(line, "DECNET OFF")) {
        decnet_enabled=0;
        puts("Periodic Phase IV Hello disabled.\n");
    } else if (eq(line, "SHOW DECNET")) {
        puts("Local area.node 1.1; Hello enabled=0x");hex32((uint32_t)decnet_enabled);
        puts(" sent=0x");hex32(decnet_hello_sent);
        puts(" observed=0x");hex32(decnet_hello_received);
        puts(" last peer=0x");hex32(decnet_peer.nodeid);
        puts(" router Hellos=0x");hex32(decnet_router_seen);
        const struct port_decnet_router_entry *best=port_decnet_adj_best(&decnet_neighbors,decnet_nodeid);
        puts(" designated router=0x");hex32(best?best->hello.nodeid:0);
        puts(" expired=0x");hex32(decnet_neighbors.expired);putc('\n');
    } else if (eq(line, "POLL ROUTER")) {
        int n=port_ne2k_x86_recv(&nic_info,nic_buffer,sizeof nic_buffer);
        decnet_last_status=n>0?
            port_decnet_parse_router_hello(nic_buffer,(unsigned)n,
                                            &decnet_router_info):-1;
        if(decnet_last_status==0) {
            port_decnet_adj_expire(&decnet_neighbors,arch_ticks());
            decnet_last_status=port_decnet_adj_update(&decnet_neighbors,
                                         &decnet_router_info,arch_ticks());
            if(decnet_last_status==0)++decnet_router_seen;
            puts("Router Hello node=0x");hex32(decnet_router_info.nodeid);
            puts(" priority=0x");hex32(decnet_router_info.priority);putc('\n');
        } else puts("No supported Router Hello.\n");
    } else if (eq(line, "POLL HELLO")) {
        int n=port_ne2k_x86_recv(&nic_info,nic_buffer,sizeof nic_buffer);
        decnet_last_status=n>0?
            port_decnet_parse_endnode_hello(nic_buffer,(unsigned)n,&decnet_peer):-1;
        if(decnet_last_status==0) {
            ++decnet_hello_received;
            puts("Endnode hello from 0x");hex32(decnet_peer.nodeid);
            puts(" blocksize=0x");hex32(decnet_peer.blocksize);
            puts(" timer=0x");hex32(decnet_peer.timer);putc('\n');
        } else puts("No supported Endnode Hello.\n");
    } else if (eq(line, "TEST NSP")) {
        uint8_t packet[20];
        uint8_t optional[3]={0,3,0};
        int n=port_nsp_build_connect(packet,sizeof packet,PORT_NSP_CI,
                                      0x1234,0,576,optional,3);
        int ok=n==12&&port_nsp_parse(packet,(unsigned)n,&nsp_last_packet)==0&&
               nsp_last_packet.source_link==0x1234&&
               nsp_last_packet.optional_length==3;
        puts(ok?"PASS: NSP Phase IV control frame encode/decode.\n":
                "FAIL: NSP control-frame diagnostic.\n");
    } else if (eq(line, "TEST SESSION")) {
        uint8_t req[PORT_SESSION_REQUEST_BYTES],obj=0;
        int n=port_session_build_request(req,sizeof req,
                                         PORT_SESSION_MIRROR_OBJECT);
        int ok=n==(int)PORT_SESSION_REQUEST_BYTES&&
               port_session_parse_request(req,(unsigned)n,&obj)==0&&
               obj==PORT_SESSION_MIRROR_OBJECT;
        puts(ok?"PASS: DECnet Session Control object 25 request.\n":
                "FAIL: Session Control request codec.\n");
    } else if (eq(line, "TEST DATA")) {
        const uint8_t ping[4]={'P','I','N','G'};
        uint8_t body[20];struct port_nsp_data check={0};
        int n=port_nsp_data_build(body,sizeof body,PORT_NSP_DATA_MOM,
                     0x1234,0x5678,0,0,1,ping,4);
        puts(n==15&&port_nsp_data_parse(body,(unsigned)n,&check)==0&&
             check.sequence==1&&check.body_len==4?
             "PASS: NSP DATA canonical frame.\n":"FAIL: NSP DATA codec.\n");
    } else if (eq(line, "TEST RELIABLE")) {
        struct port_nsp_reliable fixture;uint16_t seq=0;
        port_nsp_reliable_init(&fixture);
        int ok=port_nsp_tx_begin(&fixture,100,&seq)==0&&seq==1&&
            port_nsp_tx_tick(&fixture,300)==1&&
            port_nsp_tx_ack(&fixture,1)==1&&
            port_nsp_rx_sequence(&fixture,1)==PORT_NSP_RX_NEW&&
            port_nsp_rx_sequence(&fixture,1)==PORT_NSP_RX_DUPLICATE;
        puts(ok?"PASS: NSP stop-and-wait ACK/retransmission.\n":
                "FAIL: NSP reliability.\n");
    } else if (eq(line, "TEST MIRROR")) {
        const uint8_t ping[4]={'P','I','N','G'};
        uint8_t request[16],response[16];
        int a=port_mirror_make_request(request,sizeof request,ping,4);
        int b=a>0?port_mirror_reply(response,sizeof response,
                                      request,(unsigned)a):-1;
        puts(a==5&&b==5&&port_mirror_verify(request,5,response,5)==0?
            "PASS: DECnet Mirror request/response.\n":
            "FAIL: DECnet Mirror codec.\n");
    } else if (eq(line, "TEST SERVER")) {
        struct port_mirror_server server;
        port_mirror_server_init(&server);
        uint8_t session[PORT_SESSION_REQUEST_BYTES];uint8_t buf[64];
        struct port_nsp_message connect={0};
        int n=port_session_build_request(session,sizeof session,25);
        int c=n>0?port_nsp_build_connect(buf,sizeof buf,PORT_NSP_CI,
                      0x3737,0,512,session,(unsigned)n):-1;
        int ok=c>0&&port_nsp_parse(buf,(unsigned)c,&connect)==0&&
            port_mirror_server_connect(&server,0x0401,0x0402,&connect)==1;
        puts(ok?"PASS: NSP passive Mirror object-25 listener.\n":
                "FAIL: NSP Mirror listener.\n");
    } else if (eq(line, "NSP SERVER ON")) {
        if(nic_last_result!=0)nic_last_result=port_ne2k_x86_probe(&nic_info);
        if(nic_last_result==0) {
            uint8_t mac[6];port_decnet_node_mac(decnet_nodeid,mac);
            nic_last_result=port_ne2k_x86_set_station(&nic_info,mac);
        }
        if(nic_last_result==0) {
            port_mirror_server_init(&nsp_server);
            nsp_server_enabled=1;decnet_enabled=1;
            puts("Passive DECnet Mirror object-25 listener enabled.\n");
        } else puts("Mirror listener: NIC unavailable.\n");
    } else if (eq(line, "NSP SERVER OFF")) {
        nsp_server_enabled=0;
        port_mirror_server_close(&nsp_server);
        puts("Passive Mirror listener disabled.\n");
    } else if (eq(line, "NSP PING")) {
        const uint8_t ping[4]={'P','I','N','G'};
        int n=port_mirror_make_request(nsp_mirror_request,
                                       sizeof nsp_mirror_request,ping,4);
        int rc=n>0?nsp_send_data(nsp_mirror_request,(unsigned)n):-1;
        if(rc==0){nsp_mirror_request_len=(unsigned)n;nsp_mirror_waiting=1;}
        puts(rc==0?"NSP object-25 Mirror request sent.\n":
             rc==-2?"NSP DATA pending ACK; send deferred.\n":
                    "NSP PING requires connected link and NIC.\n");
    } else if (eq(line, "NSP OPEN")) {
        if(nic_last_result!=0)nic_last_result=port_ne2k_x86_probe(&nic_info);
        if(nic_last_result==0) {
            uint8_t mac[6];port_decnet_node_mac(decnet_nodeid,mac);
            nic_last_result=port_ne2k_x86_set_station(&nic_info,mac);
        }
        int rc=nic_last_result==0?
            port_nsp_link_open(&nsp_link,decnet_nodeid,0x0402u,0x3737u,
                               arch_ticks()):-1;
        if(rc==0) {
            nsp_data_last_sequence=nsp_data_tx_sequence=0;
            port_nsp_reliable_init(&nsp_transport);nsp_replay_len=0;
            nsp_mirror_waiting=0;
            rc=nsp_send_probe(PORT_NSP_CI);
            if(rc==0)decnet_enabled=1; /* RX and retransmits need polling */
        }
        if(rc!=0)port_nsp_link_reset(&nsp_link);
        puts(rc==0?"NSP Mirror-object connect probe sent to node 1.2.\n":
                   "NSP probe failed/busy or NIC unavailable.\n");
    } else if (eq(line, "NSP CLOSE")) {
        int rc=port_nsp_link_disconnect(&nsp_link,arch_ticks());
        if(rc==0) {
            int n=port_nsp_build_disconnect(nsp_payload,sizeof nsp_payload,
                PORT_NSP_DI,nsp_link.remote_link,nsp_link.local_link,0);
            int wire=n>0?port_decnet_build_short(nic_buffer,sizeof nic_buffer,
                nsp_link.local_node,nsp_link.remote_node,nsp_payload,(unsigned)n):-1;
            rc=wire>0?port_ne2k_x86_send(&nic_info,nic_buffer,(unsigned)wire):-1;
        }
        puts(rc==0?"NSP disconnect request sent.\n":"No connected NSP probe.\n");
    } else if (eq(line, "NSP RESET")) {
        port_nsp_link_reset(&nsp_link);
        nsp_data_last_sequence=nsp_data_tx_sequence=0;
        port_nsp_reliable_init(&nsp_transport);nsp_replay_len=0;
        nsp_mirror_waiting=0;
        puts("NSP diagnostic link reset.\n");
    } else if (eq(line, "NSP POLL")) {
        int n=port_ne2k_x86_recv(&nic_info,nic_buffer,sizeof nic_buffer);
        if(n>0&&port_decnet_parse_short(nic_buffer,(unsigned)n,&decnet_rx)==0) {
            nsp_process_short(&decnet_rx);
            puts("NSP candidate received. SHOW NSP for state.\n");
        } else puts("No supported NSP frame.\n");
    } else if (eq(line, "SHOW NSP")) {
        puts("NSP state=0x");hex32((uint32_t)nsp_link.state);
        puts(" local link=0x");hex32(nsp_link.local_link);
        puts(" remote node=0x");hex32(nsp_link.remote_node);
        puts(" remote link=0x");hex32(nsp_link.remote_link);putc('\n');
        puts("NSP attempts=0x");hex32(nsp_link.attempts);
        puts(" accepted=0x");hex32(nsp_link.received);
        puts(" peer-rejected=0x");hex32(nsp_link.rejected);
        puts(" timed-out=0x");hex32(nsp_link.timeouts);putc('\n');
        puts("NSP TX=0x");hex32(nsp_tx_count);
        puts(" TX failures=0x");hex32(nsp_tx_errors);putc('\n');
        puts("NSP data rx=0x");hex32(nsp_data_received);
        puts(" invalid=0x");hex32(nsp_data_rejected);
        puts(" ack tx=0x");hex32(nsp_dataacks_sent);
        puts(" ack rx=0x");hex32(nsp_dataacks_received);putc('\n');
        puts("NSP control messages parsed=0x");hex32(nsp_parsed);
        puts(" rejected=0x");hex32(nsp_rejected);
        puts(" last flags=0x");hex32(nsp_last_packet.flags);putc('\n');
        puts("NSP pending=0x");hex32(nsp_transport.pending);
        puts(" seq=0x");hex32(nsp_transport.pending_seq);
        puts(" retries=0x");hex32(nsp_transport.retries);
        puts(" timeouts=0x");hex32(nsp_transport.timeouts);putc('\n');
        puts("NSP duplicate=0x");hex32(nsp_transport.duplicates);
        puts(" out-of-order=0x");hex32(nsp_transport.out_of_order);
        puts(" bad ACK=0x");hex32(nsp_transport.rejected_ack);
        puts(" fragments unsupported=0x");hex32(nsp_rx_partial_unsupported);putc('\n');
        puts("Mirror ok=0x");hex32(nsp_mirror_ok);
        puts(" invalid=0x");hex32(nsp_mirror_bad);
        puts(" timeouts=0x");hex32(nsp_mirror_timeouts);
        puts(" waiting=0x");hex32(nsp_mirror_waiting);putc('\n');
        puts("Server enabled=0x");hex32((uint32_t)nsp_server_enabled);
        puts(" active=0x");hex32(nsp_server.active);
        puts(" accepts=0x");hex32(nsp_server.accepts);
        puts(" mirrors=0x");hex32(nsp_server.echoed);putc('\n');
        puts("Server duplicates=0x");hex32(nsp_server.transport.duplicates);
        puts(" retries=0x");hex32(nsp_server.transport.retries);
        puts(" rejected=0x");hex32(nsp_server.rejects);
        puts(" bad=0x");hex32(nsp_server.bad_data);putc('\n');
        puts("NSP partial transport, no full VMS DECnet.\n");
    } else if (eq(line, "TEST DECNET")) {
        uint16_t other;
        uint8_t mac[6];
        int rc=port_decnet_node(1,2,&other);
        const uint8_t sample[2]={0x24,0x01};
        int length=port_decnet_build_short(nic_buffer,sizeof nic_buffer,
                                            decnet_nodeid,other,sample,2);
        if(rc||length<0 || port_decnet_parse_short(nic_buffer,(unsigned)length,&decnet_rx))
            decnet_last_status=-1;
        else {
            port_decnet_node_mac(decnet_nodeid,mac);
            if(nic_last_result!=0)nic_last_result=port_ne2k_x86_probe(&nic_info);
            decnet_last_status=nic_last_result==0?
                port_ne2k_x86_set_station(&nic_info,mac):nic_last_result;
        }
        puts(decnet_last_status==0?"DECnet Phase IV MAC + short routing frame: PASS.\n":
             "DECnet: NIC/packet setup FAIL.\n");
    } else if (eq(line, "DECNET TX")) {
        const uint8_t sample[2]={0x24,0x01};
        uint8_t mac[6];
        port_decnet_node_mac(decnet_nodeid,mac);
        if(nic_last_result!=0)nic_last_result=port_ne2k_x86_probe(&nic_info);
        if(nic_last_result==0)nic_last_result=port_ne2k_x86_set_station(&nic_info,mac);
        int n=port_decnet_build_short(nic_buffer,sizeof nic_buffer,
                                      decnet_nodeid,0x0402u,sample,2);
        decnet_last_status=nic_last_result==0&&n>0?
            port_ne2k_x86_send(&nic_info,nic_buffer,(unsigned)n):-1;
        puts(decnet_last_status==0?"DECnet EtherType 6003 frame TX done (test payload only).\n":
             "DECnet TX failed.\n");
    } else if (eq(line, "POLL DECNET")) {
        int n=port_ne2k_x86_recv(&nic_info,nic_buffer,sizeof nic_buffer);
        decnet_last_status=n>0?
            port_decnet_parse_short(nic_buffer,(unsigned)n,&decnet_rx):-1;
        if(decnet_last_status==0) {
            puts("DECnet short: source=0x");hex32(decnet_rx.src_node);
            puts(" destination=0x");hex32(decnet_rx.dst_node);
            puts(" payload len=0x");hex32(decnet_rx.payload_len);putc('\n');
        } else puts("No supported DECnet short frame.\n");
    } else if (eq(line, "TEST NETIO")) {
        if(nic_last_result!=0)nic_last_result=port_ne2k_x86_probe(&nic_info);
        if(nic_last_result!=0){puts("FAIL: NIC probe.\n");}
        else {
            for(unsigned i=0;i<60;i++)nic_buffer[i]=0;
            for(unsigned i=0;i<6;i++)nic_buffer[i]=0xff;
            for(unsigned i=0;i<6;i++)nic_buffer[6+i]=nic_info.mac[i];
            nic_buffer[12]=0x88;nic_buffer[13]=0xb5;
            nic_buffer[14]='V';nic_buffer[15]='M';nic_buffer[16]='S';
            nic_last_result=port_ne2k_x86_send(&nic_info,nic_buffer,60);
            puts(nic_last_result==0 ? "TX OK: test Ethernet frame dispatched.\n" :
                 "FAIL: NE2000 remote-DMA transmit.\n");
        }
    } else if (eq(line, "POLL NIC")) {
        nic_rx_result=port_ne2k_x86_recv(&nic_info,nic_buffer,sizeof nic_buffer);
        puts("NE2000 receive bytes=0x");hex32((uint32_t)nic_rx_result);
        putc('\n');
    } else if (eq(line, "SHOW NIC")) {
        puts("NE2000 status=0x"); hex32((uint32_t)nic_last_result);
        puts(" reset count=0x"); hex32(nic_info.reset_count); putc('\n');
        if (nic_last_result==0) {
            puts("Station address: ");
            for (unsigned i=0;i<6;i++) {
                hex8(nic_info.mac[i]);
                if (i!=5) putc(':');
            }
            puts(" pending RX frames=0x");
            hex32((uint32_t)port_ne2k_x86_pending(&nic_info));
            puts(" tx=0x");hex32(nic_info.tx_packets);
            puts(" rx=0x");hex32(nic_info.rx_packets);
            puts(" drops=0x");hex32(nic_info.rx_drops);
            putc('\n');
        }
    } else if (eq(line, "TEST DISK")) {
        puts(ata_test()==0 ?
             "PASS: native ATA PIO LBA1/LBA2, signature and checksum.\n" :
             "FAIL: no ATA disk, timeout, or sector data mismatch.\n");
    } else if (eq(line, "SHOW DISK")) {
        puts("ATA PIO reads=0x"); hex32(ata_reads);
        puts(" last result=0x"); hex32((uint32_t)ata_last_result);
        puts(" checksum=0x"); hex32(ata_last_checksum); putc('\n');
    } else if (eq(line, "TEST EVENT")) {
        arch_run_event_test();
        puts(arch_event_blocks()==1 && arch_event_wakes()==1 &&
             arch_event_immediate()==1 && arch_event_flags()==3 &&
             arch_event_report(0)==0xEFA37001u &&
             arch_event_report(1)==0xEFB37002u &&
             arch_sched_done()==2 ?
             "PASS: event AND block/wakeup, OR immediate, 2 CPL3 tasks.\n" :
             "FAIL: event flag/scheduler invariants.\n");
    } else if (eq(line, "SHOW EVENT")) {
        puts("Common flags=0x"); hex32(arch_event_flags());
        puts(" blocks=0x"); hex32(arch_event_blocks());
        puts(" wakes=0x"); hex32(arch_event_wakes());
        puts(" immediate=0x"); hex32(arch_event_immediate()); putc('\n');
    } else if (eq(line, "TEST DEMAND")) {
        arch_run_demand_test();
        puts(arch_demand_faults()==4 && arch_demand_zero_seen() &&
             arch_demand_isolated() && arch_demand_reclaimed() ?
             "PASS: 4 demand faults, zeroed pages, CR3 isolation, reclaim.\n" :
             "FAIL: demand-page invariants.\n");
    } else if (eq(line, "SHOW VM")) {
        puts("Demand faults=0x"); hex32(arch_demand_faults());
        puts(" zeroed=0x"); hex32(arch_demand_zero_seen());
        puts(" isolated=0x"); hex32(arch_demand_isolated());
        puts(" reclaimed=0x"); hex32(arch_demand_reclaimed()); putc('\n');
    } else if (eq(line, "TEST KSTACK")) {
        arch_run_scheduler_test();
        puts(arch_sched_done()==2 && arch_kstack_checks()>=8 &&
             arch_kstack_entries(0)>=4 && arch_kstack_entries(1)>=4 &&
             arch_kstack_faults()==0 && arch_kstack_protected() ?
             "PASS: per-PCB TSS.esp0, ring3 traps, guarded kernel stacks.\n" :
             "FAIL: per-process interrupt stack invariant.\n");
    } else if (eq(line, "SHOW KSTACK")) {
        puts("Ring3 kernel stack checks=0x"); hex32(arch_kstack_checks());
        puts(" violations=0x"); hex32(arch_kstack_faults()); putc('\n');
        puts("PCB0 entries=0x"); hex32(arch_kstack_entries(0));
        puts(" PCB1 entries=0x"); hex32(arch_kstack_entries(1)); putc('\n');
        puts("TSS.esp0=0x"); hex32(arch_kstack_tss_esp0());
        puts(" guards/canaries="); puts(arch_kstack_protected()?"OK\n":"BAD\n");
    } else if (eq(line, "SHOW MEMORY")) {
        puts("Physical 4K frames free=0x"); hex32(arch_phys_free());
        puts(" high-water allocated=0x"); hex32(arch_phys_high_water());
        putc('\n');
    } else if (eq(line, "SHOW PCB")) {
        for (unsigned i=0;i<2;i++) {
            puts("PCB pid=0x"); hex32(arch_pcb_pid(i));
            puts(" state=0x"); hex32(arch_pcb_state(i));
            puts(" CR3=0x"); hex32(arch_pcb_cr3(i)); putc('\n');
            puts(" ticks=0x"); hex32(arch_pcb_ticks(i));
            puts(" dispatches=0x"); hex32(arch_pcb_dispatches(i)); putc('\n');
        }
    } else if (eq(line, "TEST ISOLATION")) {
        arch_run_isolation_test();
        puts(arch_isolation_faults()==1 && arch_isolation_intact() ?
             "PASS: other task stack rejected by user #PF.\n" :
             "FAIL: private stack isolation.\n");
    } else if (eq(line, "SHOW SCHED")) {
        puts("CPL3 task switches=0x"); hex32(arch_sched_switches());
        puts(" reports=0x"); hex32(arch_sched_reports());
        puts(" exits=0x"); hex32(arch_sched_done()); putc('\n');
        puts("Task sequence=0x"); hex32(arch_sched_sequence()); putc('\n');
        puts("Timer preemptions=0x"); hex32(arch_sched_preemptions());
        puts(" user AST reports=0x"); hex32(arch_sched_auto_ast_reports());
        puts(" AST completions=0x"); hex32(arch_sched_auto_ast_completed()); putc('\n');
    } else if (eq(line, "SHOW PRIVILEGE")) {
        puts("TSS selector=0x48, syscall gate=0x80, paging=ON.\n");
        puts("User CPL=0x"); hex32(arch_user_cpl());
        puts(" syscalls=0x"); hex32(arch_user_syscalls());
        puts(" faults=0x"); hex32(arch_user_faults());
        puts(" exits=0x"); hex32(arch_user_exits()); putc('\n');
        puts("User AST last=0x"); hex32(arch_user_ast_value()); putc('\n');
        puts("Preemption test uses separate CR3 and private user stack pages.\n");
    } else if (eq(line, "SHOW TIMER")) {
        puts("IRQ0 ticks: 0x"); hex32(arch_ticks()); putc('\n');
        puts("Last interrupt vector: 0x"); hex32(arch_last_vector()); putc('\n');
    } else if (eq(line, "TEST QIO")) {
        port_event_init(&qio_events);
        vms_ast_init(&qio_ast_process,0x370025u);
        port_qio_init(&qio_service,qio_ata_read,0);
        qio_request=(struct port_qio_request){0};
        qio_ast_seen=0;
        int rc=port_qio_submit(&qio_service,&qio_request,&qio_iosb,
                     qio_sector,1,2,&qio_ast_process,
                     qio_completion_callback,0,0x0a57u);
        if(rc==PORT_QIO_OK)rc=port_qio_run_one(&qio_service,&qio_events,
                                               &qio_ast_process);
        if(rc==1)rc=vms_ast_dispatch_one(&qio_ast_process,VMS_MODE_USER);
        int pass=rc==VMS_AST_OK&&
            qio_iosb.status==PORT_QIO_IOSB_SUCCESS&&
            qio_iosb.byte_count==512&&
            port_event_satisfied(&qio_events,2,1)&&
            qio_ast_seen==0x0a57u&&
            port_qio_release(&qio_request)==0;
        puts(pass?"PASS: QIO 512-byte read, event flag and completion AST.\n":
                  "FAIL: QIO block read/event/AST (check IDE disk).\n");
    } else if (eq(line, "TEST CHANNEL")) {
        port_event_init(&qio_events);
        vms_ast_init(&qio_ast_process,0x370026u);
        port_qio_init(&qio_service,qio_ata_read,0);
        port_channels_init(&qio_channels);
        qio_request=(struct port_qio_request){0};
        qio_ast_seen=0;
        uint32_t handle=0;
        int rc=port_channel_assign(&qio_channels,qio_ast_process.pid,
                                   qio_ata_read,0,&handle);
        qio_last_channel=handle;
        if(rc==0)rc=port_channel_qio_submit(&qio_channels,&qio_service,handle,
                    &qio_request,&qio_iosb,qio_sector,1,2,&qio_ast_process,
                    qio_completion_callback,0,0x0026u);
        if(rc==0)rc=port_channel_deassign(&qio_channels,handle,qio_ast_process.pid);
        int held=rc==PORT_CHANNEL_BUSY;
        if(held)rc=port_qio_run_one(&qio_service,&qio_events,&qio_ast_process);
        if(rc==1)rc=vms_ast_dispatch_one(&qio_ast_process,VMS_MODE_USER);
        if(rc==VMS_AST_OK)rc=port_channel_qio_release(&qio_channels,&qio_request);
        if(rc==0)rc=port_channel_deassign(&qio_channels,handle,qio_ast_process.pid);
        int passed=held&&rc==0&&qio_ast_seen==0x26u&&
                   qio_iosb.status==PORT_QIO_IOSB_SUCCESS&&
                   qio_iosb.byte_count==512&&
                   port_event_satisfied(&qio_events,2,1)&&
                   port_channel_check(&qio_channels,handle,qio_ast_process.pid)
                   ==PORT_CHANNEL_INVALID;
        puts(passed?"PASS: owned QIO channel, busy/deassign/AST/IOSB.\n":
                    "FAIL: channel QIO (check IDE disk).\n");
    } else if (eq(line, "TEST USERPTR")) {
        arch_run_userptr_test();
        int pass=arch_user_ptr_result()==15u&&arch_user_ptr_calls()==4u;
        puts(pass?"PASS: CPL3 checked pointer flags and address space.\n":
                  "FAIL: CPL3 syscall pointer validation.\n");
    } else if (eq(line, "TEST USRCOPYIN")) {
        arch_run_user_copyin_test();
        int pass=arch_user_copyin_flags()==15u &&
                 arch_user_copyin_successes()==2u &&
                 arch_user_copyin_matched()==1u;
        puts(pass?"PASS: CPL3 translated user copy-in and pointer rejects.\n":
                  "FAIL: CPL3 user copy-in.\n");
    } else if (eq(line, "SHOW USRCOPYIN")) {
        puts("CPL3 copy-in flags=0x");hex32(arch_user_copyin_flags());
        puts(" good=0x");hex32(arch_user_copyin_successes());
        puts(" match=0x");hex32(arch_user_copyin_matched());putc('\n');
    } else if (eq(line, "TEST USRCHAN")) {
        arch_run_user_channel_test();
        int pass=arch_user_channel_result()==15u&&
                 arch_user_channel_assigned()==1u&&
                 arch_user_channel_deassigned()==1u;
        puts(pass?"PASS: CPL3 assign/deassign + stale/ptr gates.\n":
                  "FAIL: CPL3 channel service.\n");
    } else if (eq(line, "TEST USRREAD")) {
        arch_run_user_qio_test();
        int pass=arch_user_read_result()==15u &&
                 arch_user_read_count()==1u &&
                 arch_user_read_bytes()==512u;
        puts(pass?"PASS: CPL3 channel-bound IDE read + validated user IOSB.\n":
                  "FAIL: CPL3 channel-bound IDE block read.\n");
    } else if (eq(line, "TEST USRDESC")) {
        arch_run_user_qio_desc_test();
        puts(arch_user_desc_result()==31u && arch_user_desc_calls()==2u ?
             "PASS: CPL3 copy-in QIO descriptor and IDE completion.\n":
             "FAIL: CPL3 user QIO descriptor.\n");
    } else if (eq(line, "SHOW USRDESC")) {
        puts("CPL3 descriptor flags=0x");hex32(arch_user_desc_result());
        puts(" submits=0x");hex32(arch_user_desc_calls());putc('\n');
    } else if (eq(line, "TEST USRCANCEL")) {
        arch_run_user_qio_cancel_test();
        int pass=arch_user_cancel_result()==63u && arch_user_cancel_calls()==1u;
        puts(pass?"PASS: CPL3 QIO cancellation and channel reclamation.\n":
                  "FAIL: CPL3 QIO cancellation.\n");
    } else if (eq(line, "SHOW USRCANCEL")) {
        puts("CPL3 QIO cancellation flags=0x");hex32(arch_user_cancel_result());
        puts(" attempts=0x");hex32(arch_user_cancel_calls());putc('\n');
    } else if (eq(line, "TEST USRTEAR")) {
        arch_run_user_qio_teardown_test();
        puts(arch_user_teardown_result()==15u ?
             "PASS: process exit reclaimed owned QIO and channel.\n":
             "FAIL: outstanding QIO not reclaimed on process exit.\n");
    } else if (eq(line, "SHOW USRTEAR")) {
        puts("CPL3 QIO process-exit flags=0x");hex32(arch_user_teardown_result());putc('\n');
    } else if (eq(line, "SHOW USRREAD")) {
        puts("User block probe=0x");hex32(arch_user_read_result());
        puts(" completions=0x");hex32(arch_user_read_count());
        puts(" bytes=0x");hex32(arch_user_read_bytes());putc('\n');
    } else if (eq(line, "SHOW USRCHAN")) {
        puts("CPL3 channel flags=0x");hex32(arch_user_channel_result());
        puts(" attempts=0x");hex32(arch_user_channel_calls());putc('\n');
    } else if (eq(line, "SHOW USERPTR")) {
        puts("User pointer result=0x");hex32(arch_user_ptr_result());
        puts(" checks=0x");hex32(arch_user_ptr_calls());putc('\n');
    } else if (eq(line, "SHOW CHANNEL")) {
        puts("Channel last=0x");hex32(qio_last_channel);
        puts(" assigned=0x");hex32(qio_channels.assigned);
        puts(" deassigned=0x");hex32(qio_channels.deassigned);putc('\n');
        puts("Channel denied=0x");hex32(qio_channels.denied);
        puts(" peak pending=0x");hex32(qio_channels.peak_outstanding);putc('\n');
    } else if (eq(line, "SHOW QIO")) {
        puts("QIO IOSB status=0x");hex32(qio_iosb.status);
        puts(" transferred=0x");hex32(qio_iosb.byte_count);
        puts(" AST marker=0x");hex32(qio_ast_seen);putc('\n');
        puts("QIO completed=0x");hex32(qio_service.completed);
        puts(" failed=0x");hex32(qio_service.failed);
        puts(" flags=0x");hex32(qio_events.flags);putc('\n');
    } else if (eq(line, "SHOW SERIAL")) {
        puts("COM1 active=0x");hex32((uint32_t)port_serial_x86_active());
        puts(" sent=0x");hex32(port_serial_x86_sent());
        puts(" dropped=0x");hex32(port_serial_x86_dropped());putc('\n');
    } else if (eq(line, "SHOW RINGS")) {
        puts("VMS modes kernel=0 exec=1 super=2 user=3.\n");
        puts("GDT ring3 + TSS + U/S page controls. TEST RING3.\n");
    } else if (eq(line, "SHOW SOURCES")) {
        puts("Reference: calmsacibis995/vms37-src (not embedded)\n");
        puts("SYS/SRC/ASTDEL.MAR and SYS/SRC/EXCEPTION.MAR\n");
    } else if (eq(line, "CLS")) {
        clear_screen();
    } else if (eq(line, "HALT")) {
        puts("Halting processor.\n");
        for (;;) __asm__ volatile ("cli; hlt");
    } else {
        puts("Unknown command. Type HELP.\n");
    }
}
/* The console QIO functions implement BOOTS/SRC/READPRMPT.MAR on the
 * same physical terminal used after startup. No emulator check is involved. */
static uint32_t bootstrap_assign(void *context,const char *device,
                                 unsigned mode,uint16_t *channel){
    (void)context;
    if(mode!=3u || !device || !eq(device,"_OPA0") ||
       !port_serial_x86_active())return 0u;
    *channel=1u;
    return 1u;
}
static uint32_t bootstrap_write(void *context,uint16_t channel,
                                const char *text,unsigned length,uint16_t *iosb){
    (void)context;
    if(channel!=1u||!text||!iosb)return 0u;
    for(unsigned j=0;j<length;j++)putc(text[j]);
    *iosb=1u;
    return 1u;
}
static uint32_t bootstrap_read(void *context,uint16_t channel,
                               char *buffer,unsigned capacity,
                               const char *prompt,unsigned prompt_length,
                               uint16_t *iosb,uint16_t *received){
    (void)context;
    if(channel!=1u||!buffer||!capacity||!iosb||!received)return 0u;
    for(unsigned i=0;i<prompt_length;i++)putc(prompt[i]);
    unsigned n=0;
    for(;;){
        char c=getkey_mode(1);
        if(c=='\n'){putc('\n');break;}
        if(c=='\b'){
            if(n){n--;putc('\b');}
        }else if(c>=' '&&c<='~'&&n<capacity){
            buffer[n++]=c;putc(c);
        }
    }
    *received=(uint16_t)n;
    *iosb=1u;
    return 1u;
}
static const struct port_boo_console_io bootstrap_qio={
    .assign=bootstrap_assign,.write=bootstrap_write,
    .readprompt=bootstrap_read
};
static struct port_boo_console bootstrap_console;
static void bootstrap_message(const char *text){
    unsigned n=0;
    while(text[n]&&n<PORT_BOO_OUTBUF_SIZE-3u){
        bootstrap_console.outbuf[n]=text[n];n++;
    }
    bootstrap_console.outlen=(uint16_t)n;
    (void)port_rio_output_line(&bootstrap_console);
}
/* The write-path regression is explicitly armed by disposable QEMU media.
 * It runs only with QEMU fw_cfg and a full marker sector, and restores it. */
static void m202_outw(uint16_t port,uint16_t value){
    __asm__ volatile("outw %w0,%w1"::"a"(value),"Nd"(port));
}
static uint16_t m202_marker_word(unsigned i){
    static const uint16_t signature[4]={0x4d56u,0x5333u,0x3753u,0x5452u};
    return i<4u?signature[i]:(uint16_t)(0xa000u|i);
}
static int m202_qemu_fw_cfg(void){
    static const char expected[4]={'Q','E','M','U'};
    m202_outw(0x510u,0u);
    for(unsigned i=0;i<4u;i++)if(inb(0x511u)!=(uint8_t)expected[i])return 0;
    return 1;
}
static int m202_ata_write_sector(void *ctx,uint32_t lbn,
                                 const uint16_t src[256]){
    (void)ctx;return port_ata_x86_write28(lbn,src);
}
static int m202_ata_read_sector(void *ctx,uint32_t lbn,uint16_t dst[256]){
    (void)ctx;return port_ata_x86_read28(lbn,dst);
}
static void m202_put16(uint8_t *p,uint16_t value){
    p[0]=(uint8_t)value;p[1]=(uint8_t)(value>>8u);
}
static void m202_put32(uint8_t *p,uint32_t value){
    m202_put16(p,(uint16_t)value);m202_put16(p+2u,(uint16_t)(value>>16u));
}
static uint32_t m202_get32(const uint8_t *p){
    return (uint32_t)p[0]|((uint32_t)p[1]<<8u)|
        ((uint32_t)p[2]<<16u)|((uint32_t)p[3]<<24u);
}
static int m202_dispatch_io(const struct port_dqddt_stage *stage,
    const struct port_dq_fdt *fdt,const uint8_t *device_db,
    const struct port_v37_device_result *device_info,uint32_t lbn,
    uint16_t data[256],uint16_t function,const struct port_dqddt_native *native,
    uint32_t (*ioc_search)(const uint8_t *,size_t,uint32_t,
       const struct port_v37_device_result *,const char *,size_t,
       struct port_v37_ioc_result *)){
    uint8_t irp[PORT_V37_IRP_SIZE] __attribute__((aligned(4)));
    if(!stage||!native||!fdt||!device_db||!device_info||!data||
       !ioc_search||!device_info->ucb[0]){
        bootstrap_message("INIT: IOC-selected DQA0 DDT request context invalid.");
        return -1;
    }
    struct port_v37_ioc_result found={0};
    if(ioc_search(device_db,PORT_V37_DEVICE_BYTES,
         device_info->list_head,device_info,"DQA0",4u,&found)!=PORT_V37_IOC_OK||
       found.ddb!=device_info->ddb[0]||found.ucb!=device_info->ucb[0]||
       found.unit!=0u||!found.driver_bound){
        bootstrap_message("INIT: IOC$SEARCHDEV did not select bound DQA0 for DQ I/O.");
        return -1;
    }
    const uint32_t ddt=m202_get32(device_db+PORT_V37_DDB_BOOT_OFF+12u);
    if(ddt!=(uint32_t)(uintptr_t)native->image||ddt!=native->base||
       m202_get32((const uint8_t *)(uintptr_t)ddt)!=stage->native_start_pc){
        bootstrap_message("INIT: DQA0 DDB DDT START pointer validation failed.");
        return -1;
    }
    typedef uint32_t (*dq_startio_fn)(const struct port_dqddt_stage *,
        const struct port_dqddt_native *,const struct port_dq_fdt *,
        const struct port_dq_bootread_request *,struct port_dq_bootread_result *);
    dq_startio_fn startio=(dq_startio_fn)(uintptr_t)
        m202_get32((const uint8_t *)(uintptr_t)ddt);
    for(unsigned i=0;i<sizeof irp;i++)irp[i]=0u;
    m202_put16(irp+8u,PORT_V37_IRP_SIZE);
    irp[10u]=PORT_V37_DYN_IRP;
    m202_put32(irp+PORT_V37_IRP_UCB,device_info->ucb[0]);
    m202_put16(irp+PORT_V37_IRP_FUNC,function);
    m202_put16(irp+42u,PORT_DQ_PHYSIO);
    m202_put16(irp+50u,PORT_DQ_BOOTREAD_BYTES);
    m202_put32(irp+PORT_V37_IRP_MEDIA,lbn);
    const struct port_dq_bootread_request request={
        .fdt=fdt,.device_db=device_db,.device_info=device_info,
        .irp=irp,.irp_bytes=sizeof irp,.sector=data,
        .sector_bytes=PORT_DQ_BOOTREAD_BYTES,.boot_unit=0u,
        .read_sector=m202_ata_read_sector,
        .write_sector=m202_ata_write_sector};
    struct port_dq_bootread_result result={0};
    if(startio(stage,native,fdt,&request,&result)!=
          PORT_DQ_BOOTREAD_OK||result.io_function!=function||
       result.lbn!=lbn||result.bytes!=PORT_DQ_BOOTREAD_BYTES||
       result.irp_status!=PORT_SYS85_NORMAL||
       result.irp_second!=PORT_DQ_BOOTREAD_BYTES){
        bootstrap_message("INIT: IOC-selected DQA0 DDT STARTIO rejected DQ I/O.");
        return -1;
    }
    return 0;
}
struct m202_dq_write_context {
    const struct port_dqddt_stage *stage;
    const struct port_dq_fdt *fdt;
    const uint8_t *device_db;
    const struct port_v37_device_result *device_info;
    const struct port_dqddt_native *native;
    uint32_t (*ioc_search)(const uint8_t *,size_t,uint32_t,
       const struct port_v37_device_result *,const char *,size_t,
       struct port_v37_ioc_result *);
    uint32_t read_lbn[2];
    uint32_t read_calls;
    uint32_t write_lbn[2];
    uint16_t write_first_word[2];
    uint32_t write_calls;
};
static uint32_t m202_dq_queued_write(void *opaque,uint32_t lbn,
                                      const uint8_t payload[512]){
    struct m202_dq_write_context *ctx=opaque;
    if(!ctx||!payload||ctx->write_calls>=2u)return PORT_DQ_IOCW_BAD;
    const unsigned call=ctx->write_calls++;
    ctx->write_lbn[call]=lbn;
    ctx->write_first_word[call]=(uint16_t)payload[0]|
        ((uint16_t)payload[1]<<8u);
    uint16_t words[256];
    for(unsigned i=0;i<256u;i++)
        words[i]=(uint16_t)payload[i*2u]|((uint16_t)payload[i*2u+1u]<<8u);
    return m202_dispatch_io(ctx->stage,ctx->fdt,ctx->device_db,
        ctx->device_info,lbn,words,PORT_DQ_WRITEPBLK,ctx->native,
        ctx->ioc_search)==0?PORT_DQ_IOCW_OK:PORT_DQ_IOCW_IO;
}
static int m202_submit_queued_write(const struct port_dqddt_stage *stage,
    const struct port_dq_fdt *fdt,const uint8_t *device_db,
    const struct port_v37_device_result *device_info,uint32_t lbn,
    const uint16_t data[256],const struct port_dqddt_native *native,
    uint32_t (*ioc_search)(const uint8_t *,size_t,uint32_t,
       const struct port_v37_device_result *,const char *,size_t,
       struct port_v37_ioc_result *)){
    struct m202_dq_write_context context={
        .stage=stage,.fdt=fdt,.device_db=device_db,.device_info=device_info,
        .native=native,.ioc_search=ioc_search};
    struct port_dq_ioc_write_queue queue;
    struct port_dq_ioc_write_iosb iosb={0};
    uint8_t first_payload[512],final_payload[512];
    for(unsigned i=0;i<256u;i++){
        const uint16_t first=(uint16_t)~data[i];
        first_payload[i*2u]=(uint8_t)first;
        first_payload[i*2u+1u]=(uint8_t)(first>>8u);
        final_payload[i*2u]=(uint8_t)data[i];
        final_payload[i*2u+1u]=(uint8_t)(data[i]>>8u);
    }
    if(port_dq_ioc_write_init(&queue,m202_dq_queued_write,&context)!=
         PORT_DQ_IOCW_OK||
       port_dq_ioc_write_prepare(&queue,1u,lbn,first_payload,
                                 sizeof first_payload)!=
         PORT_DQ_IOCW_OK||
       port_dq_ioc_write_prepare(&queue,2u,lbn,final_payload,
                                 sizeof final_payload)!=
         PORT_DQ_IOCW_OK||
       port_dq_ioc_write_enqueue(&queue,1u)!=PORT_DQ_IOCW_OK||
       port_dq_ioc_write_enqueue(&queue,2u)!=PORT_DQ_IOCW_OK||
       port_dq_ioc_write_service(&queue,&iosb)!=PORT_DQ_IOCW_OK||
       iosb.tag!=1u||iosb.status!=PORT_DQ_IOCW_OK||iosb.bytes!=512u||
       port_dq_ioc_write_service(&queue,&iosb)!=PORT_DQ_IOCW_OK||
       iosb.tag!=2u||iosb.status!=PORT_DQ_IOCW_OK||iosb.bytes!=512u||
       queue.submitted!=2u||queue.completed!=2u||queue.failed!=0u||
       context.write_calls!=2u||context.write_lbn[0]!=lbn||
       context.write_lbn[1]!=lbn||
       context.write_first_word[0]!=(uint16_t)((uint16_t)first_payload[0]|
          ((uint16_t)first_payload[1]<<8u))||
       context.write_first_word[1]!=(uint16_t)((uint16_t)final_payload[0]|
          ((uint16_t)final_payload[1]<<8u)))
        return -1;
    return 0;
}
static uint32_t m202_dq_queued_read(void *opaque,uint32_t lbn,
                                    uint8_t destination[512]){
    struct m202_dq_write_context *ctx=opaque;
    if(!ctx||!destination||ctx->read_calls>=2u)return 0u;
    ctx->read_lbn[ctx->read_calls++]=lbn;
    uint16_t words[256];
    if(m202_dispatch_io(ctx->stage,ctx->fdt,ctx->device_db,
        ctx->device_info,lbn,words,PORT_DQ_READPBLK,ctx->native,
        ctx->ioc_search)!=0)return 0u;
    for(unsigned i=0;i<256u;i++){
        destination[i*2u]=(uint8_t)words[i];
        destination[i*2u+1u]=(uint8_t)(words[i]>>8u);
    }
    return 1u;
}
static int m202_read_queued_sector(const struct port_dqddt_stage *stage,
    const struct port_dq_fdt *fdt,const uint8_t *device_db,
    const struct port_v37_device_result *device_info,uint32_t lbn,
    uint8_t destination[512],const uint16_t expected_boot[256],
    const struct port_dqddt_native *native,
    uint32_t (*ioc_search)(const uint8_t *,size_t,uint32_t,
       const struct port_v37_device_result *,const char *,size_t,
       struct port_v37_ioc_result *)){
    struct m202_dq_write_context context={
        .stage=stage,.fdt=fdt,.device_db=device_db,.device_info=device_info,
        .native=native,.ioc_search=ioc_search};
    struct port_dq_ioc_queue queue;
    struct port_dq_ioc_report report={0};
    uint8_t second[512];
    if(!destination||port_dq_ioc_init(&queue,m202_dq_queued_read,&context)!=
         PORT_DQ_IOC_OK||
       port_dq_ioc_prepare(&queue,2u,lbn,destination,512u)!=PORT_DQ_IOC_OK||
       port_dq_ioc_set_retry(&queue,2u,2u)!=PORT_DQ_IOC_OK||
       port_dq_ioc_enqueue(&queue,2u)!=PORT_DQ_IOC_OK||
       port_dq_ioc_prepare(&queue,3u,0u,second,sizeof second)!=PORT_DQ_IOC_OK||
       port_dq_ioc_set_retry(&queue,3u,2u)!=PORT_DQ_IOC_OK||
       port_dq_ioc_enqueue(&queue,3u)!=PORT_DQ_IOC_OK||
       port_dq_ioc_run(&queue,&report)!=PORT_DQ_IOC_OK||
       report.processed!=2u||report.succeeded!=2u||report.failed!=0u||
       report.reads!=2u||queue.completed!=2u||queue.posted!=2u||
       queue.active!=PORT_DQ_IOC_DEPTH||queue.reads!=2u)
        return -1;
    if(context.read_calls!=2u||context.read_lbn[0]!=lbn||
       context.read_lbn[1]!=0u)return -1;
    if(!expected_boot)return -1;
    for(unsigned i=0;i<256u;i++)
        if(expected_boot[i]!=(uint16_t)((uint16_t)second[i*2u]|
             ((uint16_t)second[i*2u+1u]<<8u)))return -1;
    for(unsigned i=0;i<PORT_DQ_IOC_DEPTH;i++)
        if(queue.packets[i].state!=PORT_DQ_IOC_EMPTY)return -1;
    return 0;
}
static int m202_ata_write_probe(const struct port_dqddt_stage *stage,
    const struct port_dq_fdt *fdt,const uint8_t *device_db,
    const struct port_v37_device_result *device_info,
    const struct port_dqddt_native *native,
    uint32_t (*ioc_search)(const uint8_t *,size_t,uint32_t,
       const struct port_v37_device_result *,const char *,size_t,
       struct port_v37_ioc_result *)){
    enum {SCRATCH_LBN=8063u};
    if(!m202_qemu_fw_cfg()){
        bootstrap_message("INIT: M202 QEMU fw_cfg signature not present; ATA write probe skipped.");
        return 0;
    }
    uint16_t saved[256],probe[256],observed[256];
    if(port_ata_x86_read28(SCRATCH_LBN,saved)){
        bootstrap_message("INIT: M202 marked ATA scratch LBN could not be read.");
        return -1;
    }
    for(unsigned i=0;i<256u;i++)if(saved[i]!=m202_marker_word(i)){
        bootstrap_message("INIT: M202 marked ATA scratch LBN marker absent; write probe skipped.");
        return 0;
    }
    for(unsigned i=0;i<256u;i++)probe[i]=(uint16_t)(0x5a00u^i);
    const int wrote=m202_submit_queued_write(stage,fdt,device_db,device_info,
        SCRATCH_LBN,probe,native,ioc_search);
    if(wrote)bootstrap_message("INIT: staged DQ WRITEPBLK action failed.");
    const int read_probe=wrote? -1:port_ata_x86_read28(SCRATCH_LBN,observed);
    if(!wrote&&read_probe)bootstrap_message("INIT: M202 ATA write readback failed.");
    int matches=!read_probe;
    if(matches)for(unsigned i=0;i<256u;i++)if(observed[i]!=probe[i]){
        matches=0;break;
    }
    const int restored=m202_submit_queued_write(stage,fdt,device_db,device_info,
        SCRATCH_LBN,saved,native,ioc_search);
    if(restored)bootstrap_message("INIT: staged DQ WRITEPBLK scratch restore failed.");
    const int reread=port_ata_x86_read28(SCRATCH_LBN,observed);
    if(!reread&&!restored)bootstrap_message("INIT: M202 ATA scratch restore reread completed.");
    int restore_matches=!reread;
    if(restore_matches)for(unsigned i=0;i<256u;i++)if(observed[i]!=saved[i]){
        restore_matches=0;break;
    }
    uint8_t through_ddt[512];
    uint16_t boot_expected[256];
    int ddt_matches=!wrote&&!restored&&!reread&&restore_matches&&
       !port_ata_x86_read28(0u,boot_expected)&&
       !m202_read_queued_sector(stage,fdt,device_db,device_info,SCRATCH_LBN,
          through_ddt,boot_expected,native,ioc_search);
    if(ddt_matches)for(unsigned i=0;i<256u;i++)
        if(saved[i]!=(uint16_t)((uint16_t)through_ddt[i*2u]|
             ((uint16_t)through_ddt[i*2u+1u]<<8u))){
            ddt_matches=0;break;
        }
    if(!wrote&&!restored&&!reread&&restore_matches&&!ddt_matches){
        bootstrap_message("INIT: published DQA0 DDT READPBLK restore verification failed.");
        return -1;
    }
    return !wrote&&matches&&!restored&&!reread&&restore_matches&&ddt_matches?1:-1;
}
/* FIL$RDWRTLBN calls the BIOS-independent boot driver. The IA-32 driver
 * reads actual blocks on the boot disk over primary IDE. */
static uint32_t bootstrap_disk_transfer(void *ctx,uint32_t mode,
                                        uint32_t function,uint32_t lbn,
                                        uint32_t count,void *buffer){
    (void)ctx;
    if(mode || function!=1u || count!=512u || !buffer)
        return BOO_IO_BADPARAM;
    return port_ata_x86_read28(lbn,(uint16_t *)buffer)==0 ?
        BOO_IO_NORMAL : BOO_IO_FAILED;
}
/* SYS.EXE, SYSLOA386.EXE and INILOA.EXE live above the 1..3 MiB pool and
 * inside the protected-mode supervisor identity aperture (0..4 MiB).
 * ELF validation checks every target interval before invoking these. */
static int bootstrap_executive_write(void *unused,uint32_t phys,
                                      const uint8_t *src,uint32_t size){
    (void)unused;
    if(!src||phys<PORT_SYS_ELF_START||phys>PORT_SYSLOA386_ELF_LIMIT||
       size>PORT_SYSLOA386_ELF_LIMIT-phys)return 0;
    volatile uint8_t *dest=(volatile uint8_t *)(uintptr_t)phys;
    for(uint32_t i=0;i<size;i++)dest[i]=src[i];
    return 1;
}
static int bootstrap_executive_clear(void *unused,uint32_t phys,uint32_t size){
    (void)unused;
    if(phys<PORT_SYS_ELF_START||phys>PORT_SYSLOA386_ELF_LIMIT||
       size>PORT_SYSLOA386_ELF_LIMIT-phys)return 0;
    volatile uint8_t *dest=(volatile uint8_t *)(uintptr_t)phys;
    for(uint32_t i=0;i<size;i++)dest[i]=0u;
    return 1;
}
/* INITADP's PC-specific CPU_ADP table invokes the *actual* protected
 * boot IDE and COM1 drivers. Disk probing reads the real Files-11 volume. */
static int native_ini_ata_read(void *ctx,uint32_t lba,uint16_t words[256]){
    (void)ctx;
    return port_ata_x86_read28(lba,words);
}
static int native_ini_uart_ready(void *ctx){
    (void)ctx;
    return port_serial_x86_active();
}
extern uint32_t arch_pit_ticks(void);
static uint32_t native_sysloa_ticks(void *ctx){
    (void)ctx;return arch_pit_ticks();
}
static struct port_f11_volume bootstrap_volume;
static void bootstrap_halt(void){
    for(;;)__asm__ volatile("cli; hlt");
}
void kmain(void) {
    (void)port_serial_x86_init();
    clear_screen();
    port_boo_console_init(&bootstrap_console,&bootstrap_qio,0);
    vms_ast_init(&ast_proc,0x370001u);
    port_condition_init(&portlab_conditions);
    if(port_condition_push(&portlab_conditions,&diagnostic_handler,
                          VMS_MODE_KERNEL,diagnostic_condition_handler,0)||
       port_condition_push(&portlab_conditions,&user_exception_handler,
                          VMS_MODE_USER,diagnostic_condition_handler,0)){
        bootstrap_message("System initialization stopped: condition handler setup failed.");
        bootstrap_halt();
    }
    arch_init();
    if(!arch_a20_ready()||!arch_ram_window_ready()){
        bootstrap_message("System initialization stopped: PC memory check failed.");
        bootstrap_halt();
    }
    if(!arch_vms_ring_probe()){
        bootstrap_message("IA-32 VMS CPL1/CPL2 IRET transition regression failed.");
        bootstrap_halt();
    }
    bootstrap_message("IA-32 VMS CPL1-to-CPL2 frame dispatch and interrupt return passed.");
    /* VMB.MAR: form the real PC PFN bitmap and RPB argument cells
     * before SYSBOOT attempts to open SYS.EXE. */
    static struct port_vmb_arguments vmb_args;
    static uint8_t vmb_bitmap[PORT_VMB_MAP_BYTES];
    const uint32_t e820_count=*(volatile uint16_t *)(uintptr_t)PORT_VMB_E820_COUNT;
    uint32_t vmb_status=port_vmb_memory_arguments(&vmb_args,vmb_bitmap,
                sizeof(vmb_bitmap),(const struct port_pc_memory_range *)
                (uintptr_t)PORT_VMB_E820_BASE,e820_count,0x00400000u);
    if(vmb_status!=PORT_VMB_OK){
        bootstrap_message("VMB: physical memory discovery failed.");
        bootstrap_halt();
    }
    /* SYSBOOT.MAR GETCURRENT first opens SYS.EXE using FIL$OPENFILE.
     * The original bootstrap file reader expects a Files-11 system disk.
     * Do not leap to INIT when this physical volume is not Files-11. */
    const struct boo_io_driver boot_disk={
        .transfer=bootstrap_disk_transfer,.ctx=0,.rpb_address=0u};
    static struct port_boot_driver_registration disk_registration;
    if(port_boot_driver_register(&disk_registration,&boot_disk,-1,1)!=PORT_VMB_OK){
        bootstrap_message("VMB: boot driver table creation failed.");
        bootstrap_halt();
    }
    const struct boo_io_driver *boot_io=0;
    if(port_boot_driver_select(&disk_registration,1,386,1,&boot_io)!=PORT_VMB_OK){
        bootstrap_message("VMB: boot disk driver selection failed.");
        bootstrap_halt();
    }
    /* VMB.MAR records the boot device, PFN descriptor and all known RAM
     * ranges in the native RPB before SYSBOOT runs. */
    static struct port_vmb_restart_block boot_rpb;
    const struct port_vmb_boot_registers pc_boot={
        .device_type=1u,.adapter_port=0x1f0u,.controller_port=0x1f7u,
        .unit=0u,.adapter_flags=0u,.software_flags=0u};
    if(port_vmb_init_restart(&boot_rpb,&vmb_args,vmb_bitmap,
            sizeof(vmb_bitmap),(const struct port_pc_memory_range *)
            (uintptr_t)PORT_VMB_E820_BASE,e820_count,boot_io,&pc_boot)!=PORT_VMB_OK){
        bootstrap_message("VMB: restart parameter block setup failed.");
        bootstrap_halt();
    }
    disk_registration.io.rpb_address=boot_rpb.base_address;
    boot_io=boot_rpb.boot_driver;
    uint32_t mount_status=port_f11_mount(boot_io,1u,&bootstrap_volume);
    if(mount_status==PORT_F11_MOUNT_DISK_ERROR){
        bootstrap_message("SYSBOOT disk read failed while mounting Files-11 volume.");
        bootstrap_halt();
    }
    if(mount_status!=PORT_F11_HOME_OK){
        bootstrap_message("SYSBOOT: system disk is not a valid Files-11 ODS-2 volume.");
        bootstrap_halt();
    }
    /* SYSBOOT.MAR GETCURRENT obtains SYS.EXE through FIL$OPENFILE.
     * File ID lookup follows MFD -> SYSEXE.DIR -> SYS.EXE. */
    static struct port_f11_fid sys_file;
    static uint8_t sys_header[512];
    uint32_t first_lbn=0,sys_blocks=0;
    uint32_t file_status=port_f11_open_named_file(boot_io,1u,
                &bootstrap_volume,"SYS$SYSDEVICE:[SYSEXE]SYS.EXE",
                &sys_file,sys_header,&first_lbn,&sys_blocks);
    if(file_status!=PORT_F11_HOME_OK){
        bootstrap_message("-F-Unable to locate SYS.EXE");
        bootstrap_halt();
    }
    /* SYSBOOT BOO$L_SYS_MAP: preserve the actual Files-11 SYS.EXE
     * allocation as 32-bit normalized retrieval pointers. Its resident
     * WCB template overlay and window consumers are still unported. */
    static struct port_f11_extent sys_extents[48];
    static uint32_t sys_normalized_words[128];
    static struct port_sysboot_normalized_map sys_normalized_map;
    uint32_t sys_extent_count=0u,sys_extent_blocks=0u;
    uint32_t sys_first_contiguous=0u;
    if(port_f11_collect_file_extents(boot_io,1u,&bootstrap_volume,
          sys_file,sys_extents,48u,&sys_extent_count,&sys_extent_blocks,
          &sys_first_contiguous)!=PORT_F11_HOME_OK ||
       !sys_extent_count || sys_extent_blocks<sys_blocks ||
       (first_lbn && sys_first_contiguous &&
        first_lbn!=sys_first_contiguous) ||
       port_sysboot_build_normalized_map((uint8_t *)sys_normalized_words,
          sizeof sys_normalized_words,sys_extents,sys_extent_count,
          sys_blocks,&sys_normalized_map)!=PORT_F11_HOME_OK ||
       port_sysboot_check_normalized_map((const uint8_t *)sys_normalized_words,
          sizeof sys_normalized_words,sys_blocks,
          &sys_normalized_map)!=PORT_F11_HOME_OK ||
       sys_normalized_map.extent_count!=sys_extent_count ||
       sys_normalized_map.total_blocks!=sys_extent_blocks){
        bootstrap_message("SYSBOOT: SYS.EXE retrieval map invalid.");
        bootstrap_halt();
    }
    bootstrap_message("SYSBOOT: Files-11 SYS.EXE normalized 32-bit retrieval map stored.");
    /* SYSBOOT LOAD_CODE: map the distinct IA-32 executive's ELF PT_LOAD
     * segments from the Files-11 file using FIL$READVBN.  Neither stage-2
     * floppy sectors nor diagnostic test functions provide SYS.EXE bytes. */
    const struct port_sys_elf_memory exec_memory={
        .context=0,.write=bootstrap_executive_write,
        .clear=bootstrap_executive_clear
    };
    uint32_t sys_entry=0u;
    uint32_t load_status=port_sysboot_load_elf(boot_io,1u,sys_header,
                                                sys_blocks,&exec_memory,&sys_entry);
    if(load_status!=PORT_SYS_ELF_OK ||
       sys_entry<PORT_SYS_ELF_START || sys_entry>=PORT_SYS_ELF_LIMIT){
        bootstrap_message("SYSBOOT: SYS.EXE native image validation or loading failed.");
        bootstrap_halt();
    }
    bootstrap_message("SYSBOOT: Files-11 SYS.EXE native image loaded.");
    /* BOOTS/SRC/SYSBOOT.MAR: carved lookaside packet pages are provided to
     * SYS.EXE before the CPU-specific load vector table is linked. These
     * counts and sizes are a temporary native startup profile, not yet read
     * from a complete SYSGEN image. No QIO process is started here. */
    static const struct port_boo_packet_settings packet_profile={
        .irp_length=PORT_V37_IRP_SIZE,.irp_count=8u,.irp_virtual_count=8u,
        .lrp_requested_size=512u,.lrp_count=2u,.lrp_virtual_count=2u,
        .srp_requested_size=128u,.srp_count=8u,.srp_virtual_count=8u};
    static struct port_sys_exec_result packet_result;
    const struct port_sys_exec_start packet_request={
        .signature=PORT_SYS_EXEC_SIGNATURE,.version=PORT_SYS_EXEC_VERSION,
        .pcb_type=PORT_SYS_EXEC_PCB_KIND,.result=&packet_result,
        .phase=PORT_SYS_EXEC_INIT_IOQUEUES,.packet_settings=&packet_profile};
    uint32_t (*initial_loaded_sys)(const struct port_sys_exec_start *)=
        (uint32_t (*)(const struct port_sys_exec_start *))(uintptr_t)sys_entry;
    if(initial_loaded_sys(&packet_request)!=PORT_SYS_EXEC_NORMAL ||
       packet_result.signature!=PORT_SYS_EXEC_PACKET_RESULT_TAG ||
       !packet_result.irp_queue || !packet_result.lrp_queue ||
       !packet_result.srp_queue ||
       packet_result.irp_count!=8u || packet_result.lrp_count!=2u ||
       packet_result.srp_count!=8u ||
       packet_result.irp_queue->next==0u ||
       packet_result.lrp_queue->next==0u ||
       packet_result.srp_queue->next==0u ||
       packet_result.irp_queue->packet_bytes!=PORT_V37_IRP_POOL_STRIDE ||
       packet_result.lrp_queue->packet_bytes!=576u ||
       packet_result.srp_queue->packet_bytes!=128u ||
       packet_result.lrp_queue->minimum_bytes!=480u){
        bootstrap_message("INIT: SYS.EXE packet queue initialization failed.");
        bootstrap_halt();
    }
    bootstrap_message("INIT: loaded SYS.EXE initialized real IRP/LRP/SRP lookaside headers.");
    /* Original SYSBOOT loads the CPU-specific SYSLOAxxx image, then INIT
     * links its self-relative entries into the resident SYS.EXE vector table
     * before loading INILOA. SYSLOA386 is the native IA-32 processor type. */
    static struct port_f11_fid sysloa_file;
    static uint8_t sysloa_header[512];
    uint32_t sysloa_first=0u,sysloa_blocks=0u;
    uint32_t sysloa_status=port_f11_open_named_file(boot_io,1u,
                  &bootstrap_volume,"SYS$SYSDEVICE:[SYSEXE]SYSLOA386.EXE",
                  &sysloa_file,sysloa_header,&sysloa_first,&sysloa_blocks);
    if(sysloa_status!=PORT_F11_HOME_OK){
        bootstrap_message("-F-Unable to locate usable SYSLOA386.EXE");
        bootstrap_halt();
    }
    uint32_t sysloa_entry=0u;
    if(port_sysboot_load_elf_range(boot_io,1u,sysloa_header,sysloa_blocks,
             &exec_memory,PORT_SYSLOA386_ELF_START,PORT_SYSLOA386_ELF_LIMIT,
             &sysloa_entry)!=PORT_SYS_ELF_OK ||
       sysloa_entry<PORT_SYSLOA386_ELF_START ||
       sysloa_entry>=PORT_SYSLOA386_ELF_LIMIT){
        bootstrap_message("SYSBOOT: SYSLOA386.EXE image validation or loading failed.");
        bootstrap_halt();
    }
    bootstrap_message("SYSBOOT: Files-11 SYSLOA386.EXE native image loaded.");
    static struct port_sysloa386_load_result sysloa_result;
    const struct port_sysloa386_load_start sysloa_start={
        .signature=PORT_SYSLOA386_SIGNATURE,.version=PORT_SYSLOA386_VERSION,
        .result=&sysloa_result};
    uint32_t (*sysloa_loaded_entry)(const struct port_sysloa386_load_start *)=
        (uint32_t (*)(const struct port_sysloa386_load_start *))
         (uintptr_t)sysloa_entry;
    if(sysloa_loaded_entry(&sysloa_start)!=PORT_SYSLOA386_OK ||
       sysloa_result.signature!=PORT_SYSLOA386_RETURN_TAG ||
       sysloa_result.descriptor_address<PORT_SYSLOA386_ELF_START ||
       sysloa_result.descriptor_bytes>PORT_SYSLOA386_ELF_LIMIT-
                  sysloa_result.descriptor_address){
        bootstrap_message("INIT: SYSLOA386 self-describing vector image invalid.");
        bootstrap_halt();
    }
    static struct port_sys_exec_result sys_link_result;
    const struct port_sys_exec_start link_request={
        .signature=PORT_SYS_EXEC_SIGNATURE,.version=PORT_SYS_EXEC_VERSION,
        .pcb_type=PORT_SYS_EXEC_PCB_KIND,.result=&sys_link_result,
        .phase=PORT_SYS_EXEC_LINK_SYSLOA,
        .sysloa_descriptor=(const void *)(uintptr_t)sysloa_result.descriptor_address,
        .sysloa_bytes=sysloa_result.descriptor_bytes};
    uint32_t (*loaded_sys)(const struct port_sys_exec_start *)=
        (uint32_t (*)(const struct port_sys_exec_start *))(uintptr_t)sys_entry;
    if(loaded_sys(&link_request)!=PORT_SYS_EXEC_NORMAL ||
       sys_link_result.signature!=PORT_SYS_EXEC_LINK_RESULT_TAG ||
       sys_link_result.sysloa_linked!=1u){
        bootstrap_message("INIT: resident SYS.EXE SYSLOA386 vector linking failed.");
        bootstrap_halt();
    }
    bootstrap_message("INIT: SYS.EXE linked live SYSLOA386 processor vectors.");
    /* Source SYSBOOT must load INILOA.EXE before INIT can call INI$IOMAP.
     * Its absence is an actual disk-file failure, not a successful INIT. */
    static struct port_f11_fid iniloa_file;
    static uint8_t iniloa_header[512];
    uint32_t iniloa_first=0u,iniloa_blocks=0u;
    uint32_t iniloa_status=port_f11_open_named_file(boot_io,1u,
                  &bootstrap_volume,"SYS$SYSDEVICE:[SYSEXE]INILOA.EXE",
                  &iniloa_file,iniloa_header,&iniloa_first,&iniloa_blocks);
    if(iniloa_status==PORT_F11_FILE_NOT_FOUND){
        bootstrap_message("-F-Unable to locate INILOA.EXE");
        bootstrap_halt();
    }
    if(iniloa_status!=PORT_F11_HOME_OK){
        bootstrap_message("SYSBOOT: INILOA.EXE directory or file header invalid.");
        bootstrap_halt();
    }
    /* Linker-produced INILOA ELF32 is separately mapped from the actual
     * Files-11 file. As with SYS.EXE, never run or claim initialization
     * before the original source-ordered INIT dependencies are present. */
    uint32_t iniloa_entry=0u;
    uint32_t iniloa_load=port_sysboot_load_elf_range(boot_io,1u,
                         iniloa_header,iniloa_blocks,&exec_memory,
                         PORT_INILOA_ELF_START,PORT_INILOA_ELF_LIMIT,
                         &iniloa_entry);
    if(iniloa_load!=PORT_SYS_ELF_OK ||
       iniloa_entry<PORT_INILOA_ELF_START||
       iniloa_entry>=PORT_INILOA_ELF_LIMIT){
        bootstrap_message("SYSBOOT: INILOA.EXE image validation or loading failed.");
        bootstrap_halt();
    }
    bootstrap_message("SYSBOOT: Files-11 INILOA.EXE native image loaded.");
    /* EXECINIT / INITADP: after the images are resident, INIT sets up the
     * CPU-specific adapter table and invokes INI$IOMAP through the loaded
     * image's entry. This is native PC I/O, not simulated VAX nexus I/O. */
    static struct port_ini_pc_bus bus;
    static struct port_ini_io_descriptor adapters[2];
    static struct port_ini_io_state adapter_state;
    bus=(struct port_ini_pc_bus){.context=0,
        .ata_read_sector=native_ini_ata_read,.uart_ready=native_ini_uart_ready,
        .ata_io_port=(uint16_t)boot_rpb.boot_registers[1],
        .uart_io_port=0x3f8u};
    uint32_t adapter_count=port_ini_pc_describe(bus.ata_io_port,
                            port_serial_x86_active()?0x3f8u:0u,adapters);
    const struct port_ini_io_ops pc_ops={&bus,port_ini_pc_probe,
                                        port_ini_pc_register};
    const struct port_iniloa_start adapter_request={
        .signature=PORT_INILOA_SIGNATURE,.version=PORT_INILOA_VERSION,
        .boot_io_base=bus.ata_io_port,.adapter_count=(uint16_t)adapter_count,
        .adapters=adapters,.driver_ops=&pc_ops,.state=&adapter_state};
    /* Indirect call is to the verified ELF32 entry in the reserved IA-32
     * supervisor range. Neither kmain nor the floppy contains that code. */
    uint32_t (*loaded_iniloa)(const struct port_iniloa_start *)=
        (uint32_t (*)(const struct port_iniloa_start *))(uintptr_t)iniloa_entry;
    if(!adapter_count || loaded_iniloa(&adapter_request)!=PORT_INI_MAP_OK ||
       !(bus.registered_mask&1u) ||
       adapter_state.boot.boot_csr_virtual!=bus.ata_io_port ||
       adapter_state.boot.boot_adapter_number>=adapter_state.num_nexus){
        bootstrap_message("INIT: INILOA adapter initialization failed.");
        bootstrap_halt();
    }
    /* Preserve the RPB's native boot I/O port in the VMB sidecar and
     * publish the original RPB$L_BOOTR1 semantics (adapter table index). */
    boot_rpb.boot_registers[1]=adapter_state.boot.boot_adapter_number;
    bootstrap_message("INIT: INILOA IOMAP executed on real IA-32 boot adapter.");

    /* INIT.MAR lock manager and realtime SPT sections follow INI$IOMAP.
     * The SPT request is zero pending the genuine SYSGEN/MMG image; in
     * this case the original INI_SPT path correctly skips reservation.
     * Numeric DYNDEF tags are likewise not invented. These are resident
     * tables only, not an operational $ENQ subsystem. */
    static const struct port_ini_lock_settings lock_settings={
        .first_free_spt=0u,.last_free_spt=0u,.realtime_spt=0u,
        .lock_id_slots=32u,.resource_hash_requested=37u,
        .max_processes=32u,.deadlock_waittime=1u};
    static struct port_sys_exec_result lock_result;
    const struct port_sys_exec_start lock_request={
        .signature=PORT_SYS_EXEC_SIGNATURE,.version=PORT_SYS_EXEC_VERSION,
        .pcb_type=PORT_SYS_EXEC_PCB_KIND,.result=&lock_result,
        .phase=PORT_SYS_EXEC_INIT_LOCKDB,.lock_settings=&lock_settings};
    if(loaded_sys(&lock_request)!=PORT_SYS_EXEC_NORMAL ||
       lock_result.signature!=PORT_SYS_EXEC_LOCK_RESULT_TAG ||
       !lock_result.lock_database ||
       lock_result.lock_max_id!=31u ||
       lock_result.lock_hash_slots!=64u ||
       lock_result.lock_process_bitmap_bytes!=5u ||
       !lock_result.lock_database->lock_id_table ||
       !lock_result.lock_database->resource_hash_table ||
       !lock_result.lock_database->process_bitmap ||
       *(const uint32_t *)(const void *)(lock_result.lock_database->lock_id_table+4u)!=2u ||
       *(const uint32_t *)(const void *)(lock_result.lock_database->lock_id_table+124u)!=0u){
        bootstrap_message("INIT: resident SYS.EXE lock database setup failed.");
        bootstrap_halt();
    }
    bootstrap_message("INIT: loaded SYS.EXE built lock ID, resource hash and deadlock bitmap tables.");

    /* INIT.MAR INI_PSTATE establishes SCH$GL_PCBVEC and SCH$GL_SEQVEC.
     * The source-derived NULL/SYSTEM resident PCB descriptors are not yet
     * runnable; no process activation, login or DCL is claimed here. */
    static const struct port_ini_proc_settings proc_settings={
        .max_processes=32u,.dyn_pcbvec_type=0u};
    static struct port_sys_exec_result proc_result;
    const struct port_sys_exec_start proc_request={
        .signature=PORT_SYS_EXEC_SIGNATURE,.version=PORT_SYS_EXEC_VERSION,
        .pcb_type=PORT_SYS_EXEC_PCB_KIND,.result=&proc_result,
        .phase=PORT_SYS_EXEC_INIT_PROCVEC,.proc_settings=&proc_settings};
    if(loaded_sys(&proc_request)!=PORT_SYS_EXEC_NORMAL ||
       proc_result.signature!=PORT_SYS_EXEC_PV_RESULT_TAG ||
       !proc_result.process_vector ||
       proc_result.process_vector->allocated_slots!=33u ||
       proc_result.process_vector->system_pix!=32u ||
       proc_result.process_vector->max_pix!=31u ||
       !proc_result.system_sched_process || !proc_result.null_sched_process ||
       proc_result.system_sched_process->pid!=32u ||
       proc_result.system_sched_process->state!=PORT_EXE_STATE_OTHER ||
       !proc_result.system_vms_pcb||
       ((uintptr_t)proc_result.system_vms_pcb&7u)!=0u||
       proc_result.system_vms_pcb[PORT_V37_PCB_SIZE]!=PORT_V37_PCB_BYTES||
       proc_result.system_vms_pcb[PORT_V37_PCB_SIZE+1u]!=0u||
       proc_result.system_vms_pcb[PORT_V37_PCB_BTYPE]!=PORT_V37_DYN_PCB||
       m202_get32(proc_result.system_vms_pcb+PORT_V37_PCB_PID)!=0x00010020u||
       m202_get32(proc_result.system_vms_pcb+PORT_V37_PCB_PHD)!=0u||
       !proc_result.null_vms_pcb||
       ((uintptr_t)proc_result.null_vms_pcb&7u)!=0u||
       proc_result.null_vms_pcb[PORT_V37_PCB_SIZE]!=PORT_V37_PCB_BYTES||
       proc_result.null_vms_pcb[PORT_V37_PCB_SIZE+1u]!=0u||
       proc_result.null_vms_pcb[PORT_V37_PCB_BTYPE]!=PORT_V37_DYN_PCB||
       m202_get32(proc_result.null_vms_pcb+PORT_V37_PCB_PID)!=0x00010000u||
       m202_get32(proc_result.null_vms_pcb+PORT_V37_PCB_PHD)!=0u||
       *(const uint32_t *)(const void *)(proc_result.process_vector->pcb_vector+128u)!=
                (uint32_t)(uintptr_t)proc_result.system_vms_pcb ||
       *(const uint32_t *)(const void *)(proc_result.process_vector->pcb_vector)!=
                (uint32_t)(uintptr_t)proc_result.null_vms_pcb){
        bootstrap_message("INIT: resident SYS.EXE PCB sequence vectors failed.");
        bootstrap_halt();
    }
    if(!proc_result.swapper_vms_pcb||!proc_result.vms_scheduler||
       m202_get32(proc_result.swapper_vms_pcb+PORT_V37_PCB_PID)!=
          0x00010001u||
       *(const uint32_t *)(const void *)(proc_result.process_vector->pcb_vector+4u)!=
          (uint32_t)(uintptr_t)proc_result.swapper_vms_pcb||
       !proc_result.vms_scheduler->initialized||
       proc_result.vms_scheduler->summary[0]!=0x80008000u||
       proc_result.vms_scheduler->summary[1]!=0u||
       proc_result.vms_scheduler->ready_count[15]!=1u||
       proc_result.vms_scheduler->ready_count[31]!=1u){
        bootstrap_message("INIT: resident SYS.EXE scheduler queues failed.");
        bootstrap_halt();
    }
    bootstrap_message("INIT: SYS.EXE queued source resident NULL/SWAPPER PCBs; PHD/context activation pending.");

    /* Original INIT.MAR INI_PHV and INI_SWAP reserve the process header
     * slots and the swapper P0 page-table map. This is *not* a running
     * swapper or paging process; the original map and refcount state are
     * inspected directly from the loaded SYS.EXE supervisor pages. */
    static const struct port_ini_phvswap_settings phv_settings={
        .balance_set_count=11u,.system_pix=32u,.max_working_set=64u,
        .phvec_dyn_type=0u,.swapmap_dyn_type=0u};
    static struct port_sys_exec_result phv_result;
    const struct port_sys_exec_start phv_request={
        .signature=PORT_SYS_EXEC_SIGNATURE,.version=PORT_SYS_EXEC_VERSION,
        .pcb_type=PORT_SYS_EXEC_PCB_KIND,.result=&phv_result,
        .phase=PORT_SYS_EXEC_INIT_PHVSWAP,.phv_settings=&phv_settings};
    if(loaded_sys(&phv_request)!=PORT_SYS_EXEC_NORMAL ||
       phv_result.signature!=PORT_SYS_EXEC_PHV_RESULT_TAG ||
       !phv_result.process_header_vector ||
       phv_result.process_header_vector->phv_slots!=12u ||
       phv_result.process_header_vector->phv_record_bytes!=60u ||
       phv_result.process_header_vector->swapmap_record_bytes!=272u ||
       phv_result.process_header_vector->swapper_p0lr!=64u ||
       *(const uint16_t *)(const void *)(phv_result.process_header_vector->phv_pix_base+22u)!=32u ||
       *(const uint16_t *)(const void *)(phv_result.process_header_vector->phv_refcount_base+22u)!=1024u ||
       *(const uint16_t *)(const void *)(phv_result.process_header_vector->phv_refcount_base+20u)!=UINT16_MAX ||
       *(const uint32_t *)(const void *)(phv_result.process_header_vector->swapper_p0br+252u)!=0u){
        bootstrap_message("INIT: resident SYS.EXE PHV and swap PTE record validation failed.");
        bootstrap_halt();
    }
    bootstrap_message("INIT: loaded SYS.EXE initialized process header vector and swapper PTE records.");

    /* INIT.MAR INI_MPW: truncate MPWPFC to a multiple of eight and add
     * seven slots for bit-level pagefile allocation. The resident SYS.EXE
     * owns both arrays. No modified-page writer process exists yet. */
    static const struct port_ini_mpw_settings mpw_settings={
        .requested_fault_cluster=32u,.dyn_mpwmap_type=0u};
    static struct port_sys_exec_result mpw_result;
    const struct port_sys_exec_start mpw_request={
        .signature=PORT_SYS_EXEC_SIGNATURE,.version=PORT_SYS_EXEC_VERSION,
        .pcb_type=PORT_SYS_EXEC_PCB_KIND,.result=&mpw_result,
        .phase=PORT_SYS_EXEC_INIT_MPW,.mpw_settings=&mpw_settings};
    if(loaded_sys(&mpw_request)!=PORT_SYS_EXEC_NORMAL ||
       mpw_result.signature!=PORT_SYS_EXEC_MPW_RESULT_TAG ||
       !mpw_result.modified_page_writer_map ||
       mpw_result.modified_page_writer_map->effective_fault_cluster!=32u ||
       mpw_result.modified_page_writer_map->swap_increment!=32u ||
       mpw_result.modified_page_writer_map->allocated_slots!=39u ||
       mpw_result.modified_page_writer_map->allocated_bytes!=246u ||
       *(const uint32_t *)(const void *)(mpw_result.modified_page_writer_map->pte_base+38u*4u)!=0u ||
       *(const uint16_t *)(const void *)(mpw_result.modified_page_writer_map->phv_index_base+38u*2u)!=0u){
        bootstrap_message("INIT: resident SYS.EXE modified-page writer record validation failed.");
        bootstrap_halt();
    }
    bootstrap_message("INIT: loaded SYS.EXE initialized modified-page writer PTE/index map.");

    /* INIT.MAR INI_PFLVEC: ask loaded SYS.EXE to select its resident
     * NULLPFL symbol. No shell PHD/WCB or real paging service exists yet.
     * This stage only constructs original-format pointer-vector storage. */
    static const struct port_ini_pflvec_settings pfl_settings={
        .page_file_count=1u,.swap_file_count=0u,.global_page_file_limit=0u,
        .ptr_dyn_type=0u,.pfl_dyn_type=0u,
        .shell_base_address=0u,.shell_system_window=0u};
    static struct port_sys_exec_result pfl_result;
    const struct port_sys_exec_start pfl_request={
        .signature=PORT_SYS_EXEC_SIGNATURE,.version=PORT_SYS_EXEC_VERSION,
        .pcb_type=PORT_SYS_EXEC_PCB_KIND,.result=&pfl_result,
        .phase=PORT_SYS_EXEC_INIT_PFLVEC,.pfl_settings=&pfl_settings};
    if(loaded_sys(&pfl_request)!=PORT_SYS_EXEC_NORMAL ||
       pfl_result.signature!=PORT_SYS_EXEC_PFL_RESULT_TAG ||
       !pfl_result.pagefile_vector ||
       pfl_result.pagefile_vector->entries!=1u ||
       pfl_result.pagefile_vector->allocated_bytes!=16u ||
       pfl_result.pagefile_vector->null_pagefile_vbn!=0u ||
       pfl_result.pagefile_vector->null_pagefile_window!=0u ||
       pfl_result.pagefile_vector->actual_swapfile_slots!=1u ||
       (uintptr_t)pfl_result.pagefile_vector<PORT_SYS_ELF_START ||
       (uintptr_t)pfl_result.pagefile_vector>
             PORT_SYS_ELF_LIMIT-sizeof(struct port_ini_pflvec) ||
       !pfl_result.pagefile_vector->pointers ||
       (uintptr_t)pfl_result.pagefile_vector->pointers<PORT_SYS_ELF_START ||
       (uintptr_t)pfl_result.pagefile_vector->pointers>PORT_SYS_ELF_LIMIT-4u ||
       !*(const uint32_t *)(const void *)pfl_result.pagefile_vector->pointers){
        bootstrap_message("INIT: resident SYS.EXE pointer vector setup failed.");
        bootstrap_halt();
    }
    bootstrap_message("INIT: loaded SYS.EXE allocated unbound pagefile pointer vector.");

    /* INIT.MAR INI_INTSTKLM (after INI_PFLVEC, pending a live PFL
     * service): use the *actual* IA-32 TSS privilege-transition stack,
     * 4096 bytes = eight original VAX 512-byte pages. Current lock extra
     * stack charge is zero because the native lock-manager service is
     * not initialized. This is a verified stack/depth record only. */
    if(!arch_kstack_protected()){
        bootstrap_message("INIT: IA-32 privilege transition stack is unprotected.");
        bootstrap_halt();
    }
    static struct port_sys_exec_intstack_settings const intstack_settings={
        .stack_pages=8u,.lock_extra_bytes=0u};
    struct port_sys_exec_intstack_settings live_stack=intstack_settings;
    live_stack.stack_high=arch_kstack_tss_esp0();
    static struct port_sys_exec_result stack_result;
    const struct port_sys_exec_start stack_request={
        .signature=PORT_SYS_EXEC_SIGNATURE,.version=PORT_SYS_EXEC_VERSION,
        .pcb_type=PORT_SYS_EXEC_PCB_KIND,.result=&stack_result,
        .phase=PORT_SYS_EXEC_INIT_INTSTKLM,
        .intstack_settings=&live_stack};
    if(loaded_sys(&stack_request)!=PORT_SYS_EXEC_NORMAL ||
       stack_result.signature!=PORT_SYS_EXEC_INTSTK_RESULT_TAG ||
       !stack_result.interrupt_stack ||
       stack_result.interrupt_stack->high!=live_stack.stack_high ||
       stack_result.interrupt_stack->low!=live_stack.stack_high-4096u ||
       stack_result.lock_max_resource_depth!=255u ||
       stack_result.interrupt_stack->max_resource_depth!=255u ||
       (uintptr_t)stack_result.interrupt_stack<PORT_SYS_ELF_START ||
       (uintptr_t)stack_result.interrupt_stack>
           PORT_SYS_ELF_LIMIT-sizeof(struct port_ini_interrupt_stack)){
        bootstrap_message("INIT: resident SYS.EXE interrupt stack depth setup failed.");
        bootstrap_halt();
    }
    bootstrap_message("INIT: loaded SYS.EXE recorded real IA-32 TSS privilege stack and lock depth.");

    /* Original INIT.MAR INI_SYSPCB: pass the actual SYSBOOT-constructed
     * BOO$L_SYS_MAP to the loaded SYS.EXE. WCB header is still the
     * documented incomplete native header, with no paging or ACP. */
    static struct port_sys_exec_result wcb_result;
    const struct port_sys_exec_wcb_settings wcb_settings={
        .normalized_boot_map=(uint8_t *)sys_normalized_words,
        .boot_map_capacity=sizeof sys_normalized_words,
        .expected_extent_count=sys_extent_count,
        .expected_first_lbn=sys_extents[0].lbn,
        .expected_total_blocks=sys_extent_blocks};
    const struct port_sys_exec_start wcb_request={
        .signature=PORT_SYS_EXEC_SIGNATURE,.version=PORT_SYS_EXEC_VERSION,
        .pcb_type=PORT_SYS_EXEC_PCB_KIND,.result=&wcb_result,
        .phase=PORT_SYS_EXEC_INIT_SYSWCB,.syswcb_settings=&wcb_settings};
    if(loaded_sys(&wcb_request)!=PORT_SYS_EXEC_NORMAL ||
       wcb_result.signature!=PORT_SYS_EXEC_SYSWCB_RESULT_TAG ||
       !wcb_result.system_wcb ||
       wcb_result.system_wcb->mapping_count!=sys_extent_count ||
       (uintptr_t)wcb_result.system_wcb<PORT_SYS_ELF_START ||
       (uintptr_t)wcb_result.system_wcb>
                    PORT_SYS_ELF_LIMIT-sizeof(struct port_ini_syswcb)){
        bootstrap_message("INIT: resident SYS.EXE SYSWCB map overlay validation failed.");
        bootstrap_halt();
    }
    bootstrap_message("INIT: loaded SYS.EXE compacted real Files-11 SYS.EXE BOO$ WCB map.");

    /* INIT.MAR 1232-1239: source-typed MB/NL/OP DPT header bytes
     * exist in SYS.EXE, but are not real linkable driver images.
     * DPT$W_SIZE=0 intentionally prevents IOC insertion. */
    static struct port_sys_exec_result dpt_result;
    const struct port_sys_exec_start dpt_request={
        .signature=PORT_SYS_EXEC_SIGNATURE,.version=PORT_SYS_EXEC_VERSION,
        .pcb_type=PORT_SYS_EXEC_PCB_KIND,.result=&dpt_result,
        .phase=PORT_SYS_EXEC_INIT_DPT_STAGES};
    if(loaded_sys(&dpt_request)!=PORT_SYS_EXEC_NORMAL ||
       dpt_result.signature!=PORT_SYS_EXEC_DPT_RESULT_TAG ||
       !dpt_result.dpt_stages ||
       (uintptr_t)dpt_result.dpt_stages<PORT_SYS_ELF_START ||
       (uintptr_t)dpt_result.dpt_stages>
                 PORT_SYS_ELF_LIMIT-3u*PORT_V37_DPT_STAGE_BYTES ||
       dpt_result.dpt_stages[0][10u]!=PORT_V37_DYN_DPT ||
       dpt_result.dpt_stages[1][10u]!=PORT_V37_DYN_DPT ||
       dpt_result.dpt_stages[2][10u]!=PORT_V37_DYN_DPT ||
       dpt_result.dpt_stages[0][8u]!=0u ||
       dpt_result.dpt_stages[1][8u]!=0u ||
       dpt_result.dpt_stages[2][8u]!=0u ||
       dpt_result.dpt_stages[0][14u]!=132u ||
       dpt_result.dpt_stages[1][14u]!=116u ||
       dpt_result.dpt_stages[2][14u]!=188u){
        bootstrap_message("INIT: resident SYS.EXE DPT prologue staging failed.");
        bootstrap_halt();
    }
    bootstrap_message("INIT: SYS.EXE staged MB/NL/OP DPT headers (driver images not linked).");

    /* NLDRIVER.MAR read/write/EOF routines were linked into SYS.EXE.
     * Check actual native text execution before linking any driver into
     * the unported IOC database. This is not a real $QIO request. */
    static struct port_sys_exec_result null_result;
    const struct port_sys_exec_start null_request={
        .signature=PORT_SYS_EXEC_SIGNATURE,.version=PORT_SYS_EXEC_VERSION,
        .pcb_type=PORT_SYS_EXEC_PCB_KIND,.result=&null_result,
        .phase=PORT_SYS_EXEC_INIT_NL_FDT};
    if(loaded_sys(&null_request)!=PORT_SYS_EXEC_NORMAL ||
       null_result.signature!=PORT_SYS_EXEC_NL_RESULT_TAG ||
       !null_result.null_driver_dpt || !null_result.null_driver_io ||
       (uintptr_t)null_result.null_driver_dpt<PORT_SYS_ELF_START ||
       (uintptr_t)null_result.null_driver_dpt>=PORT_SYS_ELF_LIMIT ||
       (uintptr_t)null_result.null_driver_io<PORT_SYS_ELF_START ||
       (uintptr_t)null_result.null_driver_io>=PORT_SYS_ELF_LIMIT ||
       null_result.null_driver_dpt->ucb_size!=116u ||
       null_result.null_driver_io(PORT_SYS85_READLBLK,0,0)!=PORT_SYS85_ENDOFFILE ||
       null_result.null_driver_io(PORT_SYS85_READVBLK,0,0)!=PORT_SYS85_ENDOFFILE ||
       null_result.null_driver_io(PORT_SYS85_WRITEOF,0,0)!=PORT_SYS85_NORMAL ||
       null_result.null_driver_io(PORT_SYS85_WRITELBLK,0,0)!=PORT_SYS85_NORMAL ||
       null_result.null_driver_io(PORT_SYS85_WRITEVBLK,0,0)!=PORT_SYS85_NORMAL ||
       null_result.null_driver_io(0u,0,0)!=PORT_SYS85_BADFUNC){
        bootstrap_message("INIT: resident SYS.EXE null driver FDT validation failed.");
        bootstrap_halt();
    }
    bootstrap_message("INIT: loaded SYS.EXE executed original NLDRIVER read/write FDT (IOC pending).");

    /* Original INIT.MAR 1240-1255 LOG$AL_DISKLOG/SYSDISKLOG. The
     * prelinked templates are source-sized and resident, now with the
     * source-verified DYN tag. DPT/TTY and SWAPINIT are unresolved. */
    static struct port_sys_exec_result log_result;
    const struct port_sys_exec_start log_request={
        .signature=PORT_SYS_EXEC_SIGNATURE,.version=PORT_SYS_EXEC_VERSION,
        .pcb_type=PORT_SYS_EXEC_PCB_KIND,.result=&log_result,
        .phase=PORT_SYS_EXEC_INIT_SYSLOG};
    if(loaded_sys(&log_request)!=PORT_SYS_EXEC_NORMAL ||
       log_result.signature!=PORT_SYS_EXEC_SYSLOG_RESULT_TAG ||
       !log_result.syslog ||
       log_result.syslog->consumed_bytes!=96u ||
       log_result.logical_table[1]!=log_result.logical_table[0]+48u ||
       (uintptr_t)log_result.syslog<PORT_SYS_ELF_START ||
       (uintptr_t)log_result.syslog>=PORT_SYS_ELF_LIMIT ||
       (uintptr_t)log_result.syslog->records[0]<PORT_SYS_ELF_START ||
       (uintptr_t)log_result.syslog->records[1]>=PORT_SYS_ELF_LIMIT ||
       log_result.syslog->records[0][20u]!=8u ||
       log_result.syslog->records[1][20u]!=13u ||
       log_result.syslog->records[0][10u]!=PORT_V37_DYN_LOG ||
       log_result.syslog->records[1][10u]!=PORT_V37_DYN_LOG){
        bootstrap_message("INIT: resident SYS.EXE logical record staging failed.");
        bootstrap_halt();
    }
    bootstrap_message("INIT: SYS.EXE staged original DYN$C_LOG=11 records (logical service pending).");

    /* Source INI_BOOTDEVIC (1311-1330): preflight the adapter list
     * published by real INILOA. Device DDB/UCB creation must wait for
     * real class and port driver images, not made-up VAX hardware. */
    static struct port_sys_exec_result bootdev_result;
    const struct port_sys_exec_start bootdev_request={
        .signature=PORT_SYS_EXEC_SIGNATURE,.version=PORT_SYS_EXEC_VERSION,
        .pcb_type=PORT_SYS_EXEC_PCB_KIND,.result=&bootdev_result,
        .phase=PORT_SYS_EXEC_INIT_BOOTDEV_SCAN,.bootdev_state=&adapter_state};
    if(loaded_sys(&bootdev_request)!=PORT_SYS_EXEC_NORMAL ||
       bootdev_result.signature!=PORT_SYS_EXEC_BOOTDEV_RESULT_TAG ||
       !bootdev_result.bootdev_scan ||
       (uintptr_t)bootdev_result.bootdev_scan<PORT_SYS_ELF_START ||
       (uintptr_t)bootdev_result.bootdev_scan>
                  PORT_SYS_ELF_LIMIT-sizeof(struct port_ini_bootdev_result) ||
       bootdev_result.bootdev_scan->boot_index!=adapter_state.boot.boot_adapter_number ||
       bootdev_result.bootdev_scan->boot_io_base!=0x1f0u ||
       bootdev_result.bootdev_scan->boot_kind!=PORT_INI_PC_ATA_PIO ||
       bootdev_result.bootdev_scan->scanned_adapters!=adapter_state.num_nexus){
        bootstrap_message("INIT: resident SYS.EXE boot adapter scan failed.");
        bootstrap_halt();
    }
    bootstrap_message("INIT: SYS.EXE selected registered ATA boot adapter (DDB/UCB pending).");

    /* Original DQDRIVER.MAR 496-590: source function masks are staged
     * in loaded SYS.EXE; ATA/RB730 hardware and file ACP actions are not
     * bound by the presence of a legal-function bit. */
    static struct port_sys_exec_result dq_result;
    const struct port_sys_exec_start dq_request={
        .signature=PORT_SYS_EXEC_SIGNATURE,.version=PORT_SYS_EXEC_VERSION,
        .pcb_type=PORT_SYS_EXEC_PCB_KIND,.result=&dq_result,
        .phase=PORT_SYS_EXEC_INIT_DQ_FDT};
    static struct port_dq_fdt_decision dq_read_decision,dq_write_decision;
    if(loaded_sys(&dq_request)!=PORT_SYS_EXEC_NORMAL ||
       dq_result.signature!=PORT_SYS_EXEC_DQ_RESULT_TAG ||
       !dq_result.dq_source_fdt || !dq_result.dq_fdt_classify ||
       (uintptr_t)dq_result.dq_source_fdt<PORT_SYS_ELF_START ||
       (uintptr_t)dq_result.dq_source_fdt>
                   PORT_SYS_ELF_LIMIT-sizeof(struct port_dq_fdt) ||
       (uintptr_t)dq_result.dq_fdt_classify<PORT_SYS_ELF_START ||
       (uintptr_t)dq_result.dq_fdt_classify>=PORT_SYS_ELF_LIMIT ||
       dq_result.dq_fdt_classify(dq_result.dq_source_fdt,
             sizeof(struct port_dq_fdt),PORT_DQ_READLBLK,
             &dq_read_decision)!=PORT_DQ_FDT_OK ||
       dq_read_decision.number!=PORT_DQ_READLBLK ||
       dq_read_decision.action_class!=PORT_DQ_FDT_READ ||
       dq_read_decision.buffered||dq_read_decision.driver_bound ||
       dq_result.dq_fdt_classify(dq_result.dq_source_fdt,
             sizeof(struct port_dq_fdt),PORT_DQ_WRITELBLK,
             &dq_write_decision)!=PORT_DQ_FDT_OK ||
       dq_write_decision.number!=PORT_DQ_WRITELBLK ||
       dq_write_decision.action_class!=PORT_DQ_FDT_WRITE ||
       dq_write_decision.buffered||dq_write_decision.driver_bound){
        bootstrap_message("INIT: resident source DQ FDT staging validation failed.");
        bootstrap_halt();
    }
    bootstrap_message("INIT: SYS.EXE staged DQDRIVER 12 source FDT masks (disk DDT and ACP unbound).");

    /* Original prelinked DEVICEDAT.MAR DDB/UCB pointers now occupy the
     * real loaded SYS.EXE. Class/port driver DDTs and CRBs are not yet
     * linked; this is a source-format prelink, NOT online VMS I/O. */
    static struct port_sys_exec_result devicedb_result;
    const struct port_sys_exec_start devicedb_request={
        .signature=PORT_SYS_EXEC_SIGNATURE,.version=PORT_SYS_EXEC_VERSION,
        .pcb_type=PORT_SYS_EXEC_PCB_KIND,.result=&devicedb_result,
        .phase=PORT_SYS_EXEC_INIT_DEVICE_DB};
    if(loaded_sys(&devicedb_request)!=PORT_SYS_EXEC_NORMAL ||
       devicedb_result.signature!=PORT_SYS_EXEC_DEVICE_RESULT_TAG ||
       !devicedb_result.device_db || !devicedb_result.device_db_bytes ||
       !devicedb_result.device_db_check || !devicedb_result.null_ddt_bytes ||
       (uintptr_t)devicedb_result.device_db_check<PORT_SYS_ELF_START ||
       (uintptr_t)devicedb_result.device_db_check>=PORT_SYS_ELF_LIMIT ||
       (uintptr_t)devicedb_result.device_db<PORT_SYS_ELF_START ||
       (uintptr_t)devicedb_result.device_db>
           PORT_SYS_ELF_LIMIT-sizeof(struct port_v37_device_result) ||
       (uintptr_t)devicedb_result.device_db_bytes<PORT_SYS_ELF_START ||
       (uintptr_t)devicedb_result.device_db_bytes>
           PORT_SYS_ELF_LIMIT-PORT_V37_DEVICE_BYTES ||
       devicedb_result.device_db->list_head!=
           (uint32_t)(uintptr_t)devicedb_result.device_db_bytes ||
       devicedb_result.device_db->incomplete_driver_mask!=7u ||
       devicedb_result.device_db->bound_nl_ddt!=devicedb_result.null_ddt_base ||
       devicedb_result.null_ddt_base!=(uint32_t)(uintptr_t)devicedb_result.null_ddt_bytes ||
       (uintptr_t)devicedb_result.null_ddt_bytes<PORT_SYS_ELF_START ||
       (uintptr_t)devicedb_result.null_ddt_bytes>
                  PORT_SYS_ELF_LIMIT-PORT_V37_NL_IMAGE_BYTES ||
       devicedb_result.null_ddt_bytes[36u]!=PORT_V37_NL_FDT_BYTES ||
       devicedb_result.null_ddt_bytes[37u]!=0u ||
       devicedb_result.null_ddt_bytes[48u]!=0u ||
       devicedb_result.device_db_check(devicedb_result.device_db_bytes,
           PORT_V37_DEVICE_BYTES,
           (uint32_t)(uintptr_t)devicedb_result.device_db_bytes,
           devicedb_result.device_db)!=PORT_V37_DEV_OK){
        bootstrap_message("INIT: SYS.EXE DEVICEDAT DDB/UCB prelink validation failed.");
        bootstrap_halt();
    }
    bootstrap_message("INIT: SYS.EXE linked NL$DDT DDTAB/FUNCTAB to NLA0 (other drivers absent).");

    /* Source DQDRIVER.MAR PREPROCESS (640-671), FDISPATCH (678-709)
     * and FUNCXT (917-938): pass the real ATA PIO callback into the
     * already loaded native SYS.EXE for a guarded *physical* READPBLK.
     * This proves one actual Files-11 home sector data path, not an
     * ACP-mediated disk driver, public $QIO, or original DEC driver load. */
    static struct port_sys_exec_result dq_bootread_result;
    const struct port_sys_exec_dq_boot_settings dq_boot_io={
        .read_sector=native_ini_ata_read,.context=0,
        .boot_unit=boot_rpb.boot_unit,.reserved=0u,
        .irq_handler=port_ata_x86_irq};
    const struct port_sys_exec_start dq_bootread_request={
        .signature=PORT_SYS_EXEC_SIGNATURE,.version=PORT_SYS_EXEC_VERSION,
        .pcb_type=PORT_SYS_EXEC_PCB_KIND,.result=&dq_bootread_result,
        .phase=PORT_SYS_EXEC_INIT_DQ_BOOTREAD,.dq_boot_io=&dq_boot_io};
    struct port_f11_home actual_dq_home={0};
    if(loaded_sys(&dq_bootread_request)!=PORT_SYS_EXEC_NORMAL ||
       dq_bootread_result.signature!=PORT_SYS_EXEC_DQ_BOOTREAD_TAG ||
       !dq_bootread_result.dq_bootread ||
       !dq_bootread_result.dq_bootread_sector ||
       (uintptr_t)dq_bootread_result.dq_bootread<PORT_SYS_ELF_START ||
       (uintptr_t)dq_bootread_result.dq_bootread>=PORT_SYS_ELF_LIMIT ||
       (uintptr_t)dq_bootread_result.dq_bootread_sector<PORT_SYS_ELF_START ||
       (uintptr_t)dq_bootread_result.dq_bootread_sector>
          PORT_SYS_ELF_LIMIT-PORT_DQ_BOOTREAD_BYTES ||
       dq_bootread_result.dq_bootread->lbn!=1u ||
       dq_bootread_result.dq_bootread->bytes!=512u ||
       dq_bootread_result.dq_bootread->irp_status!=1u ||
       dq_bootread_result.dq_bootread->irp_second!=512u ||
       dq_bootread_result.dq_bootread->io_function!=PORT_DQ_READPBLK ||
       dq_bootread_result.dq_bootread->unit!=0u ||
       dq_bootread_result.dq_bootread->sector_checksum!=
               dq_bootread_result.dq_bootread_sector[255u] ||
       port_f11_home_read((const uint8_t *)dq_bootread_result.dq_bootread_sector,
                         512u,1u,&actual_dq_home)!=PORT_F11_HOME_OK ||
       actual_dq_home.home_lbn!=1u || actual_dq_home.cluster==0u){
        bootstrap_message("INIT: resident DQ physical boot READPBLK failed.");
        bootstrap_halt();
    }
    bootstrap_message("INIT: SYS.EXE DQ READPBLK fetched actual ATA Files-11 LBN1 (disk DDT/ACP not linked).");
    if(!dq_bootread_result.dq_bootzero||!dq_bootread_result.dq_bootzero_sector||
       (uintptr_t)dq_bootread_result.dq_bootzero<PORT_SYS_ELF_START||
       (uintptr_t)dq_bootread_result.dq_bootzero>=PORT_SYS_ELF_LIMIT-
           sizeof(struct port_dq_bootread_result)||
       (uintptr_t)dq_bootread_result.dq_bootzero_sector<PORT_SYS_ELF_START||
       (uintptr_t)dq_bootread_result.dq_bootzero_sector>
           PORT_SYS_ELF_LIMIT-512u||
       dq_bootread_result.dq_bootzero->lbn!=0u||
       dq_bootread_result.dq_bootzero->bytes!=512u||
       dq_bootread_result.dq_bootzero->irp_status!=PORT_SYS85_NORMAL||
       dq_bootread_result.dq_bootzero->ucb_io_count!=2u){
        bootstrap_message("INIT: resident DQ physical LBN0 READPBLK failed.");
        bootstrap_halt();
    }
    bootstrap_message("INIT: SYS.EXE DQ READPBLK fetched real ATA LBN0 in second privileged IRP (no public QIO).");


    /* FIL$MOUNT: INDEXF.SYS header must be loaded by the resident
     * native physical DQ path, not a copied SYSBOOT disk sector. */
    uint32_t index_lbn=actual_dq_home.index_bitmap_lbn+
                       actual_dq_home.index_bitmap_blocks;
    if(index_lbn<actual_dq_home.index_bitmap_lbn||
       !dq_bootread_result.dq_index_header||
       !dq_bootread_result.dq_index_sector||
       (uintptr_t)dq_bootread_result.dq_index_header<PORT_SYS_ELF_START||
       (uintptr_t)dq_bootread_result.dq_index_header>
           PORT_SYS_ELF_LIMIT-sizeof(struct port_dq_index_result)||
       (uintptr_t)dq_bootread_result.dq_index_sector<PORT_SYS_ELF_START||
       (uintptr_t)dq_bootread_result.dq_index_sector>
           PORT_SYS_ELF_LIMIT-512u||
       dq_bootread_result.dq_index_header->lbn!=index_lbn||
       dq_bootread_result.dq_index_header->file_number!=1u||
       dq_bootread_result.dq_index_header->file_sequence!=1u||
       dq_bootread_result.dq_index_header->io_count!=3u||
       port_dq_index_validate_header(dq_bootread_result.dq_index_sector,
           &(struct port_dq_index_result){0})!=PORT_DQ_INDEX_OK){
        bootstrap_message("INIT: resident SYS.EXE INDEXF.SYS physical FH2 read failed.");
        bootstrap_halt();
    }
    bootstrap_message("INIT: SYS.EXE DQ READPBLK verified real ATA INDEXF.SYS FH2 in third privileged IRP (ACP unbound).");

    /* FIL$RDCHKFILHDR must derive MFD's physical location through
     * INDEXF.SYS mapping in the separately loaded SYS.EXE. */
    uint32_t mfd_vbn=0u,mfd_lbn=0u;
    if(port_dq_mfd_location((const uint8_t *)dq_bootread_result.dq_bootread_sector,
             (const uint8_t *)dq_bootread_result.dq_index_sector,
             &mfd_vbn,&mfd_lbn)!=PORT_DQ_MFD_OK||
       !dq_bootread_result.dq_mfd_header||!dq_bootread_result.dq_mfd_sector||
       (uintptr_t)dq_bootread_result.dq_mfd_header<PORT_SYS_ELF_START||
       (uintptr_t)dq_bootread_result.dq_mfd_header>
           PORT_SYS_ELF_LIMIT-sizeof(struct port_dq_mfd_result)||
       (uintptr_t)dq_bootread_result.dq_mfd_sector<PORT_SYS_ELF_START||
       (uintptr_t)dq_bootread_result.dq_mfd_sector>PORT_SYS_ELF_LIMIT-512u||
       dq_bootread_result.dq_mfd_header->vbn!=mfd_vbn||
       dq_bootread_result.dq_mfd_header->lbn!=mfd_lbn||
       dq_bootread_result.dq_mfd_header->fid!=4u||
       dq_bootread_result.dq_mfd_header->sequence!=4u||
       dq_bootread_result.dq_mfd_header->io_count!=4u||
       port_dq_mfd_validate(dq_bootread_result.dq_mfd_sector,
           &(struct port_dq_mfd_result){0})!=PORT_DQ_MFD_OK){
        bootstrap_message("INIT: resident SYS.EXE MFD FH2 indexed physical read failed.");
        bootstrap_halt();
    }
    bootstrap_message("INIT: SYS.EXE DQ READPBLK verified real ATA MFD FID 4,4,0 via INDEXF.SYS extent map (ACP unbound).");

    /* FIL$FINDFILID after actual MFD -> SYSEXE.DIR -> SYS.EXE reads,
     * all performed by resident SYS.EXE with separate privileged IRPs. */
    if(!dq_bootread_result.dq_sysexe_dir_fid||
       !dq_bootread_result.dq_sysexecutable_fid||
       (uintptr_t)dq_bootread_result.dq_sysexe_dir_fid<PORT_SYS_ELF_START||
       (uintptr_t)dq_bootread_result.dq_sysexe_dir_fid>
              PORT_SYS_ELF_LIMIT-sizeof(struct port_dq_dir_fid)||
       (uintptr_t)dq_bootread_result.dq_sysexecutable_fid<PORT_SYS_ELF_START||
       (uintptr_t)dq_bootread_result.dq_sysexecutable_fid>
              PORT_SYS_ELF_LIMIT-sizeof(struct port_dq_dir_fid)||
       dq_bootread_result.dq_sysexe_dir_fid->number!=10u||
       dq_bootread_result.dq_sysexe_dir_fid->sequence!=10u||
       dq_bootread_result.dq_sysexecutable_fid->number!=11u||
       dq_bootread_result.dq_sysexecutable_fid->sequence!=11u||
       dq_bootread_result.dq_dir_irp_last_count!=7u){
        bootstrap_message("INIT: resident SYS.EXE Files-11 MFD/SYSEXE directory traversal failed.");
        bootstrap_halt();
    }
    bootstrap_message("INIT: SYS.EXE native FIL$FINDFILID traversed real MFD and SYSEXE.DIR to SYS.EXE FID 11 (ACP unbound).");

    /* A real physically read SYS.EXE FH2 and ELF32 sector, separate
     * from boot monitor loading and verified as native IA-32. */
    if(!dq_bootread_result.dq_image_probe||
       (uintptr_t)dq_bootread_result.dq_image_probe<PORT_SYS_ELF_START||
       (uintptr_t)dq_bootread_result.dq_image_probe>
                    PORT_SYS_ELF_LIMIT-sizeof(struct port_dq_image_result)||
       dq_bootread_result.dq_image_probe->file_number!=11u||
       dq_bootread_result.dq_image_probe->file_sequence!=11u||
       dq_bootread_result.dq_image_probe->aux_bytes_verified<1500u||
       dq_bootread_result.dq_image_probe->aux_sectors_verified<4u||
       dq_bootread_result.dq_image_probe->range_bytes_verified!=1024u||
       dq_bootread_result.dq_image_probe->range_irps!=3u||
       dq_bootread_result.dq_image_probe->path_bytes_verified!=768u||
       dq_bootread_result.dq_image_probe->path_irps!=7u||
       dq_bootread_result.dq_image_probe->path_entries_verified<3u||
       dq_bootread_result.dq_image_probe->chain_bytes_verified!=777u||
       dq_bootread_result.dq_image_probe->chain_irps!=4u||
       dq_bootread_result.dq_image_probe->chain_headers_verified!=1u||
       dq_bootread_result.dq_image_probe->cursor_bytes_verified!=901u||
       dq_bootread_result.dq_image_probe->cursor_irps!=4u||
       dq_bootread_result.dq_image_probe->private_ioc_reads!=3u||
       dq_bootread_result.dq_image_probe->private_ioc_bytes!=1536u||
       dq_bootread_result.dq_image_probe->f11_ioc_reads!=2u||
       dq_bootread_result.dq_image_probe->f11_ioc_bytes!=700u||
       dq_bootread_result.dq_image_probe->f11_channel_reads!=2u||
       dq_bootread_result.dq_image_probe->f11_channel_bytes!=600u||
       dq_bootread_result.dq_image_probe->irp_count!=13u+
             dq_bootread_result.dq_image_probe->text_sectors_verified+
             dq_bootread_result.dq_image_probe->aux_sectors_verified+
             dq_bootread_result.dq_image_probe->range_irps+
             dq_bootread_result.dq_image_probe->path_irps+
             dq_bootread_result.dq_image_probe->chain_irps+
             dq_bootread_result.dq_image_probe->cursor_irps+
             dq_bootread_result.dq_image_probe->private_ioc_reads+
             dq_bootread_result.dq_image_probe->f11_ioc_reads+
             dq_bootread_result.dq_image_probe->f11_channel_reads+2u||
       dq_bootread_result.dq_image_probe->text_va!=PORT_SYS_ELF_START||
       dq_bootread_result.dq_image_probe->text_file_offset!=4096u||
       dq_bootread_result.dq_image_probe->text_bytes_verified!=dq_bootread_result.dq_image_probe->text_elf_filesz||
       dq_bootread_result.dq_image_probe->text_sectors_verified<16u||
       dq_bootread_result.dq_image_probe->text_sectors_verified>512u||
       dq_bootread_result.dq_image_probe->elf_type!=2u||
       !dq_bootread_result.dq_image_probe->elf_segments||
       dq_bootread_result.dq_image_probe->entry<PORT_SYS_ELF_START||
       dq_bootread_result.dq_image_probe->entry>=PORT_SYS_ELF_LIMIT){
        bootstrap_message("INIT: resident SYS.EXE physical ELF32 source probe failed.");
        bootstrap_halt();
    }
    bootstrap_message("INIT: SYS.EXE FIL$READVBN verified real ATA SYS.EXE FH2 and native ELF32 via IRPs 8 and 9 (activation pending).");

    bootstrap_message("INIT: SYS.EXE FIL$READVBN verified complete native executable PT_LOAD text against resident SYS.EXE (activation pending).");
    bootstrap_message("INIT: SYS.EXE verified native SYSLOA386.EXE and INILOA.EXE text from real Files-11 ATA sectors (ACP unbound).");
    bootstrap_message("INIT: SYS.EXE FIL$READVBN verified unaligned 1024-byte range through three real ATA IRPs (ACP unbound).");
    bootstrap_message("INIT: SYS.EXE native FIL$FINDFILID reopened SYS.EXE through real MFD/SYSEXE/FH2 and verified 768 text bytes (ACP unbound).");
    bootstrap_message("INIT: SYS.EXE native FIL$FINDFILID enumerated real SYSEXE.DIR image FIDs via ATA (ACP unbound).");
    bootstrap_message("INIT: SYS.EXE FIL$RDCHKFILHDR collected source FH2 extent chain and verified 777 bytes via ATA (ACP unbound).");
    bootstrap_message("INIT: SYS.EXE FIL$READVBN cursor compared 901 resident bytes through four physical ATA IRPs (ACP unbound).");
    bootstrap_message("INIT: SYS.EXE source DQ_STARTIO private IOC queue completed three FIFO ATA IRPs with real resident text (DDT and ACP unbound).");
    bootstrap_message("INIT: SYS.EXE RETRYERR/IOCIOPOST posted three source FIFO ATA IOSBs after live resident reads (no DQA0 DDT or IRQ).");
    bootstrap_message("INIT: SYS.EXE FIL$READVBN used IOC private FIFO for two real ATA reads and matched 700 bytes (ACP/IRQ unbound).");
    bootstrap_message("INIT: SYS.EXE private owner-scoped channel assigned, read 600 live ATA bytes and deassigned (ACP/$QIO unbound).");
    if(!port_ata_x86_irq_count()){
        bootstrap_message("INIT: primary ATA IRQ14 did not complete a sector read.");
        bootstrap_halt();
    }
    bootstrap_message("INIT: SYS.EXE ATA sector reads completed through live primary IRQ14.");

    /* Original INIT.MAR FIX_DRV_NAME/FIN_DATABASE (1500-1559): after
     * the actual Files-11 boot home read, populate source-compatible DQA
     * DDB and both nonpaged counted logical-name record equivalences.
     * These records are *not* a live LOG$TRANSLATE or usable DQ DDT. */
    static struct port_sys_exec_result finish_names_result;
    const struct port_sys_exec_start finish_names_request={
        .signature=PORT_SYS_EXEC_SIGNATURE,.version=PORT_SYS_EXEC_VERSION,
        .pcb_type=PORT_SYS_EXEC_PCB_KIND,.result=&finish_names_result,
        .phase=PORT_SYS_EXEC_INIT_FIN_DATABASE};
    if(loaded_sys(&finish_names_request)!=PORT_SYS_EXEC_NORMAL ||
       finish_names_result.signature!=PORT_SYS_EXEC_FIN_DATABASE_TAG ||
       !finish_names_result.boot_device_names ||
       !finish_names_result.device_db || !finish_names_result.device_db_bytes ||
       !finish_names_result.syslog ||
       (uintptr_t)finish_names_result.boot_device_names<PORT_SYS_ELF_START ||
       (uintptr_t)finish_names_result.boot_device_names>
            PORT_SYS_ELF_LIMIT-sizeof(struct port_ini_fin_result) ||
       (uintptr_t)finish_names_result.device_db_bytes<PORT_SYS_ELF_START ||
       (uintptr_t)finish_names_result.device_db_bytes>
            PORT_SYS_ELF_LIMIT-PORT_V37_DEVICE_BYTES ||
       finish_names_result.boot_device_names->records_staged!=1u ||
       finish_names_result.boot_device_names->boot_unit!=boot_rpb.boot_unit ||
       finish_names_result.boot_device_names->boot_ddb!=
            devicedb_result.device_db->ddb[0] ||
       finish_names_result.boot_device_names->boot_ucb!=
            devicedb_result.device_db->ucb[0] ||
       finish_names_result.boot_device_names->physical_count!=6u ||
       finish_names_result.boot_device_names->system_count!=7u ||
       finish_names_result.device_db->boot_name_ready!=1u ||
       finish_names_result.device_db->incomplete_driver_mask!=7u ||
       (finish_names_result.device_db_bytes[12u]|
        finish_names_result.device_db_bytes[13u]|
        finish_names_result.device_db_bytes[14u]|
        finish_names_result.device_db_bytes[15u])!=0u ||
       finish_names_result.device_db_bytes[20u]!=3u ||
       finish_names_result.device_db_bytes[21u]!='D' ||
       finish_names_result.device_db_bytes[22u]!='Q' ||
       finish_names_result.device_db_bytes[23u]!='A' ||
       finish_names_result.device_db_bytes[36u]!=8u ||
       finish_names_result.syslog!=log_result.syslog ||
       finish_names_result.syslog->records[0]!=log_result.syslog->records[0] ||
       finish_names_result.syslog->records[1]!=log_result.syslog->records[1] ||
       finish_names_result.syslog->records[0][29u]!=6u ||
       finish_names_result.syslog->records[1][34u]!=7u ||
       finish_names_result.syslog->records[0][30u]!='_' ||
       finish_names_result.syslog->records[0][31u]!='D' ||
       finish_names_result.syslog->records[0][32u]!='Q' ||
       finish_names_result.syslog->records[0][33u]!='A' ||
       finish_names_result.syslog->records[0][34u]!='0' ||
       finish_names_result.syslog->records[0][35u]!=':' ||
       finish_names_result.syslog->records[1][35u]!='_' ||
       finish_names_result.syslog->records[1][36u]!='_' ||
       finish_names_result.syslog->records[1][37u]!='D' ||
       finish_names_result.syslog->records[1][38u]!='Q' ||
       finish_names_result.syslog->records[1][39u]!='A' ||
       finish_names_result.syslog->records[1][40u]!='0' ||
       finish_names_result.syslog->records[1][41u]!=':'){
        bootstrap_message("INIT: resident FIN_DATABASE source boot DDB/logical record staging failed.");
        bootstrap_halt();
    }
    bootstrap_message("INIT: SYS.EXE FIN_DATABASE staged DQA0 names and resident SYS$DISK/SYS$SYSDEVICE (logical lookup pending).");

    /* Original LOGNAMSUB.MAR SYSTEM-only bootstrap LOG$HASH and
     * LOG$TRANSLATE. Two real resident source records, no process/group
     * tables, no general $TRNLOG, mount, ACP or working disk device. */
    static struct port_sys_exec_result logsys_result;
    const struct port_sys_exec_start logsys_request={
        .signature=PORT_SYS_EXEC_SIGNATURE,.version=PORT_SYS_EXEC_VERSION,
        .pcb_type=PORT_SYS_EXEC_PCB_KIND,.result=&logsys_result,
        .phase=PORT_SYS_EXEC_INIT_SYSTEM_LOG};
    static uint8_t namebuf[24];
    struct port_log_result name_result={0};
    if(loaded_sys(&logsys_request)!=PORT_SYS_EXEC_NORMAL||
       logsys_result.signature!=PORT_SYS_EXEC_SYSTEM_LOG_TAG||
       !logsys_result.boot_system_log_table||
       !logsys_result.boot_system_translate||
       (uintptr_t)logsys_result.boot_system_log_table<PORT_SYS_ELF_START||
       (uintptr_t)logsys_result.boot_system_log_table>
           PORT_SYS_ELF_LIMIT-sizeof(struct port_log_system_table)||
       (uintptr_t)logsys_result.boot_system_translate<PORT_SYS_ELF_START||
       (uintptr_t)logsys_result.boot_system_translate>=PORT_SYS_ELF_LIMIT||
       logsys_result.boot_system_log_table->count!=2u||
       logsys_result.boot_system_log_table->hash_bits!=PORT_LOG_BITS||
       logsys_result.boot_system_log_table->ready!=1u||
       logsys_result.boot_system_translate(logsys_result.boot_system_log_table,
          "SYS$DISK",8u,0u,namebuf,sizeof namebuf,
          &name_result)!=PORT_LOG_OK||
       name_result.bytes!=6u||namebuf[0]!='_'||namebuf[1]!='D'||
       namebuf[2]!='Q'||namebuf[3]!='A'||namebuf[4]!='0'||namebuf[5]!=':'||
       logsys_result.boot_system_translate(logsys_result.boot_system_log_table,
          "SYS$SYSDEVICE",13u,0u,namebuf,sizeof namebuf,
          &name_result)!=PORT_LOG_OK||
       name_result.bytes!=7u||namebuf[0]!='_'||namebuf[1]!='_'||
       namebuf[2]!='D'||namebuf[3]!='Q'||namebuf[4]!='A'||
       namebuf[5]!='0'||namebuf[6]!=':'||
       logsys_result.boot_system_translate(logsys_result.boot_system_log_table,
          "_DQA0:",6u,0u,namebuf,sizeof namebuf,
          &name_result)!=PORT_LOG_NOTRAN||
       name_result.bytes!=5u||namebuf[0]!='D'||namebuf[1]!='Q'||
       namebuf[2]!='A'||namebuf[3]!='0'||namebuf[4]!=':'){
        bootstrap_message("INIT: resident source LOGNAMSUB bootstrap translation failed.");
        bootstrap_halt();
    }
    bootstrap_message("INIT: SYS.EXE LOG$TRANSLATE resolved original SYS$DISK and SYS$SYSDEVICE names (system bootstrap only).");

    /* CONINTDSP.MAR action transitions execute in native SYS.EXE on a
     * verified OPA0 source-layout copy. No class/port DDT or UART QIO. */
    static struct port_sys_exec_result con_result;
    const struct port_sys_exec_start con_request={
        .signature=PORT_SYS_EXEC_SIGNATURE,.version=PORT_SYS_EXEC_VERSION,
        .pcb_type=PORT_SYS_EXEC_PCB_KIND,.result=&con_result,
        .phase=PORT_SYS_EXEC_INIT_CONPORT};
    if(loaded_sys(&con_request)!=PORT_SYS_EXEC_NORMAL ||
       con_result.signature!=PORT_SYS_EXEC_CON_RESULT_TAG ||
       !con_result.console_action_scratch ||
       con_result.console_action_result!=PORT_V37_CON_DEFERRED ||
       (uintptr_t)con_result.console_action_scratch<PORT_SYS_ELF_START ||
       (uintptr_t)con_result.console_action_scratch>
           PORT_SYS_ELF_LIMIT-PORT_V37_OPA_UCB_SIZE){
        bootstrap_message("INIT: resident CONINTDSP source port action checks failed.");
        bootstrap_halt();
    }
    bootstrap_message("INIT: SYS.EXE executed console XON/XOFF/STOP/RESUME transitions (TT driver absent).");

    /* IOSUBPAGD.MAR IOC$SEARCHDEV: execute a bounded physical-device
     * name walk in genuine resident SYS.EXE without faking IOC$GL_DEVLIST
     * publication, LOG$TRANSLATE, `$ASSIGN`, QIO or terminal output. */
    static struct port_sys_exec_result ioc_result;
    const struct port_sys_exec_start ioc_request={
        .signature=PORT_SYS_EXEC_SIGNATURE,.version=PORT_SYS_EXEC_VERSION,
        .pcb_type=PORT_SYS_EXEC_PCB_KIND,.result=&ioc_result,
        .phase=PORT_SYS_EXEC_INIT_IOC_LOOKUP};
    if(loaded_sys(&ioc_request)!=PORT_SYS_EXEC_NORMAL ||
       ioc_result.signature!=PORT_SYS_EXEC_IOC_RESULT_TAG ||
       !ioc_result.ioc_samples || !ioc_result.ioc_search ||
       (uintptr_t)ioc_result.ioc_samples<PORT_SYS_ELF_START ||
       (uintptr_t)ioc_result.ioc_samples>
          PORT_SYS_ELF_LIMIT-3u*sizeof(struct port_v37_ioc_result) ||
       (uintptr_t)ioc_result.ioc_search<PORT_SYS_ELF_START ||
       (uintptr_t)ioc_result.ioc_search>=PORT_SYS_ELF_LIMIT ||
       ioc_result.ioc_samples[0].ucb!=devicedb_result.device_db->ucb[1] ||
       ioc_result.ioc_samples[1].ucb!=devicedb_result.device_db->ucb[2] ||
       ioc_result.ioc_samples[2].ucb!=devicedb_result.device_db->ucb[4] ||
       !ioc_result.ioc_samples[0].concealed ||
       ioc_result.ioc_samples[0].driver_bound ||
       ioc_result.ioc_samples[1].driver_bound ||
       !ioc_result.ioc_samples[2].driver_bound){
        bootstrap_message("INIT: resident SYS.EXE physical IOC name search validation failed.");
        bootstrap_halt();
    }
    bootstrap_message("INIT: SYS.EXE IOC device search resolved NLA0 with native null DDT (others unbound).");

    /* Original IOC$SEARCHDEV: resolve the actual bootstrap SYSTEM logical
     * names to original prelinked physical UCBs without granting channels. */
    static struct port_sys_exec_result ioc_logical_result;
    const struct port_sys_exec_start ioc_logical_request={
        .signature=PORT_SYS_EXEC_SIGNATURE,.version=PORT_SYS_EXEC_VERSION,
        .pcb_type=PORT_SYS_EXEC_PCB_KIND,.result=&ioc_logical_result,
        .phase=PORT_SYS_EXEC_INIT_IOC_LOGICAL};
    if(loaded_sys(&ioc_logical_request)!=PORT_SYS_EXEC_NORMAL||
       ioc_logical_result.signature!=PORT_SYS_EXEC_IOC_LOGICAL_TAG||
       !ioc_logical_result.ioc_logical_samples||
       !ioc_logical_result.ioc_logical_search||
       (uintptr_t)ioc_logical_result.ioc_logical_samples<PORT_SYS_ELF_START||
       (uintptr_t)ioc_logical_result.ioc_logical_samples>
          PORT_SYS_ELF_LIMIT-3u*sizeof(struct port_devlog_trace)||
       (uintptr_t)ioc_logical_result.ioc_logical_search<PORT_SYS_ELF_START||
       (uintptr_t)ioc_logical_result.ioc_logical_search>=PORT_SYS_ELF_LIMIT||
       ioc_logical_result.ioc_logical_samples[0].ioc_status!=PORT_V37_IOC_UNBOUND||
       ioc_logical_result.ioc_logical_samples[1].ioc_status!=PORT_V37_IOC_UNBOUND||
       ioc_logical_result.ioc_logical_samples[2].ioc_status!=PORT_V37_IOC_OK||
       ioc_logical_result.ioc_logical_samples[0].device.ucb!=devicedb_result.device_db->ucb[0]||
       ioc_logical_result.ioc_logical_samples[1].device.ucb!=devicedb_result.device_db->ucb[0]||
       ioc_logical_result.ioc_logical_samples[2].device.ucb!=devicedb_result.device_db->ucb[4]||
       !ioc_logical_result.ioc_logical_samples[1].concealed||
       ioc_logical_result.ioc_logical_samples[0].driver_bound||
       ioc_logical_result.ioc_logical_samples[1].driver_bound||
       !ioc_logical_result.ioc_logical_samples[2].driver_bound){
        bootstrap_message("INIT: resident IOC logical to physical boot name lookup failed.");
        bootstrap_halt();
    }
    bootstrap_message("INIT: SYS.EXE IOC$SEARCHDEV resolved SYS$DISK/SYS$SYSDEVICE to unbound DQA0 and NLA0 to linked DDT (no $ASSIGN).");

    /* Source DQDRIVER.MAR DPTAB/DDTAB/FUNCTAB: native supervisor table
     * layout staging only, no DQA0 DDT binding or Files-11 ACP. */
    static struct port_sys_exec_result dq_ddt_result;
    const struct port_sys_exec_start dq_ddt_request={
        .signature=PORT_SYS_EXEC_SIGNATURE,.version=PORT_SYS_EXEC_VERSION,
        .pcb_type=PORT_SYS_EXEC_PCB_KIND,.result=&dq_ddt_result,
        .phase=PORT_SYS_EXEC_INIT_DQ_DDT_STAGE,.dq_boot_io=&dq_boot_io};
    if(loaded_sys(&dq_ddt_request)!=PORT_SYS_EXEC_NORMAL||
       dq_ddt_result.signature!=PORT_SYS_EXEC_DQ_DDT_TAG||
       !dq_ddt_result.dq_ddt_stage||!dq_ddt_result.dq_ddt_function||
       !dq_ddt_result.dq_ddt_dispatch_bootwrite||
       !dq_ddt_result.dq_ddt_startio||
       !dq_ddt_result.dq_crb||
       (uintptr_t)dq_ddt_result.dq_ddt_stage<PORT_SYS_ELF_START||
       (uintptr_t)dq_ddt_result.dq_ddt_stage>
           PORT_SYS_ELF_LIMIT-sizeof(struct port_dqddt_stage)||
       (uintptr_t)dq_ddt_result.dq_ddt_function<PORT_SYS_ELF_START||
       (uintptr_t)dq_ddt_result.dq_ddt_function>=PORT_SYS_ELF_LIMIT||
       (uintptr_t)dq_ddt_result.dq_ddt_dispatch_bootwrite<PORT_SYS_ELF_START||
       (uintptr_t)dq_ddt_result.dq_ddt_dispatch_bootwrite>=PORT_SYS_ELF_LIMIT||
       (uintptr_t)dq_ddt_result.dq_ddt_startio<PORT_SYS_ELF_START||
       (uintptr_t)dq_ddt_result.dq_ddt_startio>=PORT_SYS_ELF_LIMIT||
       (uintptr_t)dq_ddt_result.dq_crb<PORT_SYS_ELF_START||
       (uintptr_t)dq_ddt_result.dq_crb>
           PORT_SYS_ELF_LIMIT-sizeof(struct port_dq_crb)||
       port_dq_crb_validate(dq_ddt_result.dq_crb)!=PORT_DQ_CRB_OK||
       !dq_ddt_result.dq_ddt_stage->ready||
       dq_ddt_result.dq_ddt_stage->driver_bound||
       dq_ddt_result.dq_ddt_stage->table_base!=
         (uint32_t)(uintptr_t)dq_ddt_result.dq_ddt_stage->image||
       dq_ddt_result.dq_ddt_stage->native_start_pc!=
         (uint32_t)(uintptr_t)dq_ddt_result.dq_ddt_startio||
       (dq_ddt_result.dq_ddt_stage->image[36u] !=
            (uint8_t)PORT_DQDDT_FDT_BYTES ||
        dq_ddt_result.dq_ddt_stage->image[37u]!=0u)||
       dq_ddt_result.dq_ddt_function(dq_ddt_result.dq_ddt_stage,
          dq_result.dq_source_fdt,PORT_DQ_READPBLK)!=PORT_DQDDT_NATIVE||
       dq_ddt_result.dq_ddt_function(dq_ddt_result.dq_ddt_stage,
          dq_result.dq_source_fdt,PORT_DQ_WRITEPBLK)!=PORT_DQDDT_NATIVE||
       dq_ddt_result.dq_ddt_function(dq_ddt_result.dq_ddt_stage,
          dq_result.dq_source_fdt,PORT_DQ_WRITELBLK)!=PORT_DQDDT_UNBOUND||
       (devicedb_result.device_db_bytes[PORT_V37_DDB_BOOT_OFF+12u]!=0u||
        devicedb_result.device_db_bytes[PORT_V37_DDB_BOOT_OFF+13u]!=0u||
        devicedb_result.device_db_bytes[PORT_V37_DDB_BOOT_OFF+14u]!=0u||
        devicedb_result.device_db_bytes[PORT_V37_DDB_BOOT_OFF+15u]!=0u)){
        bootstrap_message("INIT: resident DQ DDT source table staging failed.");
        bootstrap_halt();
    }
    if(!arch_disk_irq_bind_crb((struct port_dq_crb *)dq_ddt_result.dq_crb)){
        bootstrap_message("INIT: native DQA0 CRB could not bind IRQ14.");
        bootstrap_halt();
    }
    bootstrap_message("INIT: SYS.EXE staged DQ DPTAB/DDTAB/FUNCTAB and native IRQ14 CRB; DQA0 DDT remains unbound.");

    /* Source RELOCDRV IOC$INITDRV: DPT_STORE program initializes a
     * verified copy of original DQA0 DDB/UCB, never the active driver. */
    static struct port_sys_exec_result relocdrv_result;
    const struct port_sys_exec_start relocdrv_request={
        .signature=PORT_SYS_EXEC_SIGNATURE,.version=PORT_SYS_EXEC_VERSION,
        .pcb_type=PORT_SYS_EXEC_PCB_KIND,.result=&relocdrv_result,
        .phase=PORT_SYS_EXEC_INIT_RELOCDRV_DQ};
    if(loaded_sys(&relocdrv_request)!=PORT_SYS_EXEC_NORMAL||
       relocdrv_result.signature!=PORT_SYS_EXEC_RELOCDRV_DQ_TAG||
       !relocdrv_result.dq_dpt_initialized||!relocdrv_result.dq_dpt_check||
       (uintptr_t)relocdrv_result.dq_dpt_initialized<PORT_SYS_ELF_START||
       (uintptr_t)relocdrv_result.dq_dpt_initialized>
          PORT_SYS_ELF_LIMIT-sizeof(struct port_relocdrv_dq)||
       (uintptr_t)relocdrv_result.dq_dpt_check<PORT_SYS_ELF_START||
       (uintptr_t)relocdrv_result.dq_dpt_check>=PORT_SYS_ELF_LIMIT||
       relocdrv_result.dq_dpt_check(relocdrv_result.dq_dpt_initialized)!=
          PORT_RELOCDRV_OK||
       relocdrv_result.dq_dpt_initialized->source_ddb!=
          devicedb_result.device_db->ddb[0]||
       relocdrv_result.dq_dpt_initialized->source_ucb!=
          devicedb_result.device_db->ucb[0]||
       relocdrv_result.dq_dpt_initialized->driver_bound||
       devicedb_result.device_db_bytes[PORT_V37_DDB_BOOT_OFF+12u]!=0u||
       devicedb_result.device_db_bytes[PORT_V37_UCB_BOOT_OFF+120u]!=0u){
        bootstrap_message("INIT: resident RELOCDRV original DQ INITDRV table failed.");
        bootstrap_halt();
    }
    bootstrap_message("INIT: SYS.EXE RELOCDRV IOC$INITDRV applied source DQ DPT_STORE to private DQA0 records (DDT/CRB unbound).");


    /* IOC$RELOC_DDT in the real resident SYS.EXE, on an isolated copy.
     * The copied zero actions are NOT valid dispatch destinations. */
    static struct port_sys_exec_result ddtreloc_result;
    const struct port_sys_exec_start ddtreloc_request={
        .signature=PORT_SYS_EXEC_SIGNATURE,.version=PORT_SYS_EXEC_VERSION,
        .pcb_type=PORT_SYS_EXEC_PCB_KIND,.result=&ddtreloc_result,
        .phase=PORT_SYS_EXEC_INIT_DDT_RELOC_PRIVATE};
    if(loaded_sys(&ddtreloc_request)!=PORT_SYS_EXEC_NORMAL||
       ddtreloc_result.signature!=PORT_SYS_EXEC_DDT_RELOC_PRIVATE_TAG||
       !ddtreloc_result.dq_relocated_private||
       !ddtreloc_result.dq_private_relocation_check||
       !ddtreloc_result.dq_native_ddt||
       !ddtreloc_result.dq_native_ddt_check||
       (uintptr_t)ddtreloc_result.dq_relocated_private<PORT_SYS_ELF_START||
       (uintptr_t)ddtreloc_result.dq_relocated_private>
         PORT_SYS_ELF_LIMIT-PORT_DQDDT_IMAGE_BYTES||
       (uintptr_t)ddtreloc_result.dq_private_relocation_check<PORT_SYS_ELF_START||
       (uintptr_t)ddtreloc_result.dq_private_relocation_check>=PORT_SYS_ELF_LIMIT||
       (uintptr_t)ddtreloc_result.dq_native_ddt<PORT_SYS_ELF_START||
       (uintptr_t)ddtreloc_result.dq_native_ddt>
         PORT_SYS_ELF_LIMIT-sizeof(struct port_dqddt_native)||
       (uintptr_t)ddtreloc_result.dq_native_ddt_check<PORT_SYS_ELF_START||
       (uintptr_t)ddtreloc_result.dq_native_ddt_check>=PORT_SYS_ELF_LIMIT||
       ddtreloc_result.dq_relocated_private_base!=
         (uint32_t)(uintptr_t)ddtreloc_result.dq_relocated_private||
       ddtreloc_result.dq_private_relocation_check(
         ddtreloc_result.dq_relocated_private,PORT_DQDDT_IMAGE_BYTES,
         ddtreloc_result.dq_relocated_private_base)!=PORT_DDTRELOC_OK||
       ddtreloc_result.dq_native_ddt->base!=
         (uint32_t)(uintptr_t)ddtreloc_result.dq_native_ddt->image||
       ddtreloc_result.dq_native_ddt_check(dq_ddt_result.dq_ddt_stage,
         dq_result.dq_source_fdt,ddtreloc_result.dq_native_ddt)!=
            PORT_DQDDT_OK||
       m202_get32(ddtreloc_result.dq_native_ddt->image)!=
         (uint32_t)(uintptr_t)dq_ddt_result.dq_ddt_startio||
       devicedb_result.device_db_bytes[PORT_V37_DDB_BOOT_OFF+12u]!=0u){
        bootstrap_message("INIT: resident private IOC$RELOC_DDT failed.");
        bootstrap_halt();
    }
    bootstrap_message("INIT: resident IOC$RELOC_DDT source DDT/FDT relocation verified (private only; DQA0 unbound).");
    bootstrap_message("INIT: SYS.EXE materialized native DQ DDT/FDT pointers (IOC publication pending).");
    /* Attach the CRB to the boot UCB only after private relocation has
     * consumed the original unbound device database state. */
    static struct port_sys_exec_result dq_crb_read_result;
    uint32_t dq_crb_irqs_before=arch_disk_irq_crb_count();
    uint32_t dq_ata_irqs_before=port_ata_x86_irq_count();
    const struct port_sys_exec_start dq_crb_read_request={
        .signature=PORT_SYS_EXEC_SIGNATURE,.version=PORT_SYS_EXEC_VERSION,
        .pcb_type=PORT_SYS_EXEC_PCB_KIND,.result=&dq_crb_read_result,
        .phase=PORT_SYS_EXEC_INIT_DQ_CRB_READ,.dq_boot_io=&dq_boot_io};
    if(loaded_sys(&dq_crb_read_request)!=PORT_SYS_EXEC_NORMAL||
       dq_crb_read_result.signature!=PORT_SYS_EXEC_DQ_CRB_READ_TAG||
       dq_crb_read_result.dq_crb!=dq_ddt_result.dq_crb||
       !dq_crb_read_result.dq_ddt_bootread||
       !dq_crb_read_result.dq_ddt_bootread_sector||
       (uintptr_t)dq_crb_read_result.dq_ddt_bootread<PORT_SYS_ELF_START||
       (uintptr_t)dq_crb_read_result.dq_ddt_bootread>
          PORT_SYS_ELF_LIMIT-sizeof(struct port_dq_bootread_result)||
       (uintptr_t)dq_crb_read_result.dq_ddt_bootread_sector<PORT_SYS_ELF_START||
       (uintptr_t)dq_crb_read_result.dq_ddt_bootread_sector>
          PORT_SYS_ELF_LIMIT-PORT_DQ_BOOTREAD_BYTES||
       dq_crb_read_result.dq_ddt_bootread->lbn!=0u||
       dq_crb_read_result.dq_ddt_bootread->bytes!=PORT_DQ_BOOTREAD_BYTES||
       dq_crb_read_result.dq_ddt_bootread->irp_status!=PORT_SYS85_NORMAL||
       dq_crb_read_result.dq_ddt_bootread->io_function!=PORT_DQ_READPBLK||
       port_ata_x86_irq_count()<=dq_ata_irqs_before||
       arch_disk_irq_crb_count()<=dq_crb_irqs_before||
       devicedb_result.device_db_bytes[PORT_V37_UCB_BOOT_OFF+32u]!=
          (uint8_t)(uintptr_t)dq_ddt_result.dq_crb||
       devicedb_result.device_db_bytes[PORT_V37_UCB_BOOT_OFF+33u]!=
          (uint8_t)((uintptr_t)dq_ddt_result.dq_crb>>8)||
       devicedb_result.device_db_bytes[PORT_V37_UCB_BOOT_OFF+34u]!=
          (uint8_t)((uintptr_t)dq_ddt_result.dq_crb>>16)||
       devicedb_result.device_db_bytes[PORT_V37_UCB_BOOT_OFF+35u]!=
          (uint8_t)((uintptr_t)dq_ddt_result.dq_crb>>24)||
       devicedb_result.device_db_bytes[PORT_V37_UCB_BOOT_OFF+120u]!=0u||
       devicedb_result.device_db_bytes[PORT_V37_DDB_BOOT_OFF+12u]!=0u){
        bootstrap_message("INIT: native DQA0 CRB IRQ14 READPBLK completion failed.");
        bootstrap_halt();
    }
    bootstrap_message("INIT: SYS.EXE native DQ DDT STARTIO completed READPBLK through CRB IRQ14.");
    bootstrap_message("INIT: SYS.EXE allocated, completed and returned DQA0 IRP through resident source lookaside.");

    /* Source NLDRIVER.MAR DDT/FDT through SYSQIOREQ.MAR 741-749: two original
     * IRP$L_MEDIA statuses and an IOC$GL_PSBL append in actual loaded
     * SYS.EXE supervisor data. IPL4 request is pending, not dispatched. */
    static struct port_sys_exec_result post_result;
    const struct port_sys_exec_start post_request={
        .signature=PORT_SYS_EXEC_SIGNATURE,.version=PORT_SYS_EXEC_VERSION,
        .pcb_type=PORT_SYS_EXEC_PCB_KIND,.result=&post_result,
        .phase=PORT_SYS_EXEC_INIT_QIO_POST};
    if(loaded_sys(&post_request)!=PORT_SYS_EXEC_NORMAL ||
       post_result.signature!=PORT_SYS_EXEC_POST_RESULT_TAG ||
       !post_result.iopost || !post_result.post_irp_bytes ||
       !post_result.nl_post ||
       (uintptr_t)post_result.iopost<PORT_SYS_ELF_START ||
       (uintptr_t)post_result.iopost>
           PORT_SYS_ELF_LIMIT-sizeof(struct port_v37_iopost_queue) ||
       (uintptr_t)post_result.post_irp_bytes<PORT_SYS_ELF_START ||
       (uintptr_t)post_result.post_irp_bytes>
           PORT_SYS_ELF_LIMIT-2u*PORT_V37_IRP_SIZE ||
       post_result.iopost->queued!=2u ||
       post_result.iopost_softint_count!=2u ||
       post_result.iopost_softint_level!=PORT_V37_IOPOST_IPL ||
       post_result.iopost_ucb_ops!=2u ||
       post_result.iopost->first!=(uint32_t)(uintptr_t)post_result.post_irp_bytes ||
       (post_result.post_irp_bytes[PORT_V37_IRP_MEDIA]!=
           (uint8_t)PORT_SYS85_ENDOFFILE ||
        post_result.post_irp_bytes[PORT_V37_IRP_MEDIA+1u]!=
           (uint8_t)(PORT_SYS85_ENDOFFILE>>8) ||
        post_result.post_irp_bytes[PORT_V37_IRP_MEDIA+2u]!=0u ||
        post_result.post_irp_bytes[PORT_V37_IRP_MEDIA+3u]!=0u) ||
       (post_result.post_irp_bytes[PORT_V37_IRP_SIZE+PORT_V37_IRP_MEDIA]!=
           PORT_SYS85_NORMAL ||
        post_result.post_irp_bytes[PORT_V37_IRP_SIZE+PORT_V37_IRP_MEDIA+1u]!=0u ||
        post_result.post_irp_bytes[PORT_V37_IRP_SIZE+PORT_V37_IRP_MEDIA+2u]!=0u ||
        post_result.post_irp_bytes[PORT_V37_IRP_SIZE+PORT_V37_IRP_MEDIA+3u]!=0u)){
        bootstrap_message("INIT: resident SYS.EXE NLDRIVER to IOC post queue failed.");
        bootstrap_halt();
    }
    bootstrap_message("INIT: SYS.EXE NLDRIVER completed two IRPs via linked DDT/FDT and EXE$FINISHIOC (IPL4 requested).");

    /* IOCIOPOST.MAR 145-165: source IRP post FIFO drains inside a
     * real CPL0 x86 interrupt gate (SCBVECTOR original slot 0x90).
     * No process channel IOSB or AST is reported as initialized. */
    static struct port_sys_exec_result end_result;
    const struct port_sys_exec_start end_request={
        .signature=PORT_SYS_EXEC_SIGNATURE,.version=PORT_SYS_EXEC_VERSION,
        .pcb_type=PORT_SYS_EXEC_PCB_KIND,.result=&end_result,
        .phase=PORT_SYS_EXEC_INIT_IOPOST_END};
    if(loaded_sys(&end_request)!=PORT_SYS_EXEC_NORMAL ||
       end_result.signature!=PORT_SYS_EXEC_IOPOST_BIND_TAG ||
       !end_result.iopost_isr ||
       end_result.iopost_remaining!=2u ||
       (uintptr_t)end_result.iopost_isr<PORT_SYS_ELF_START ||
       (uintptr_t)end_result.iopost_isr>=PORT_SYS_ELF_LIMIT ||
       arch_iopost_bind(end_result.iopost_isr)!=1u ||
       arch_iopost_trigger()!=1u ||
       arch_iopost_count()!=1u ||
       arch_iopost_last_status()!=PORT_V37_IOPOST_OK){
        bootstrap_message("INIT: resident IOCIOPOST native software interrupt failed.");
        bootstrap_halt();
    }
    static struct port_sys_exec_result verify_end_result;
    const struct port_sys_exec_start verify_end_request={
        .signature=PORT_SYS_EXEC_SIGNATURE,.version=PORT_SYS_EXEC_VERSION,
        .pcb_type=PORT_SYS_EXEC_PCB_KIND,.result=&verify_end_result,
        .phase=PORT_SYS_EXEC_VERIFY_IOPOST_INT};
    if(loaded_sys(&verify_end_request)!=PORT_SYS_EXEC_NORMAL ||
       verify_end_result.signature!=PORT_SYS_EXEC_IOPOST_END_TAG ||
       !verify_end_result.iopost_endlog || !verify_end_result.iopost ||
       verify_end_result.iopost_endlog->completed!=2u ||
       verify_end_result.iopost_endlog->status[0]!=PORT_SYS85_ENDOFFILE ||
       verify_end_result.iopost_endlog->status[1]!=PORT_SYS85_NORMAL ||
       verify_end_result.iopost_endlog->irp_va[0]!=(uint32_t)(uintptr_t)post_result.post_irp_bytes ||
       verify_end_result.iopost_endlog->irp_va[1]!=
           (uint32_t)(uintptr_t)post_result.post_irp_bytes+PORT_V37_IRP_SIZE ||
       verify_end_result.iopost_remaining!=0u ||
       verify_end_result.iopost!=post_result.iopost ||
       verify_end_result.iopost->first!=verify_end_result.iopost->sentinel ||
       verify_end_result.iopost->last!=verify_end_result.iopost->sentinel){
        bootstrap_message("INIT: resident IOCIOPOST source kernel end-action processing failed.");
        bootstrap_halt();
    }
    bootstrap_message("INIT: SYS.EXE IOCIOPOST serviced two NLA IRPs through IA-32 CPL0 software INT 0x90 (no process AST).");
    /* Resident SYS.EXE clock, process queues and SCB setup execute from the
     * image read through Files-11. This is not a full system startup. */
    uint64_t native_now=0u;
    if(!port_rtc_x86_now(0,&native_now)){
        bootstrap_message("INIT: hardware system clock is unavailable.");
        bootstrap_halt();
    }
    static const struct port_sysloa386_cpu_ops cpu_ops={
        .context=0,.read_irq_ticks=native_sysloa_ticks};
    static const struct port_sysloa386_boot_ops boot_ops={
        .context=0,.read_sector=native_ini_ata_read};
    static struct port_sys_exec_result sys_result;
    const struct port_sys_exec_start sys_request={
        .signature=PORT_SYS_EXEC_SIGNATURE,.version=PORT_SYS_EXEC_VERSION,
        .system_time=native_now,.pcb_type=PORT_SYS_EXEC_PCB_KIND,
        .initialized_images=3u,.reserved=0u,.result=&sys_result,
        .phase=PORT_SYS_EXEC_INIT_RESIDENT,
        .cpu_ops=&cpu_ops,.boot_ops=&boot_ops};
    if(loaded_sys(&sys_request)!=PORT_SYS_EXEC_NORMAL ||
       sys_result.signature!=PORT_SYS_EXEC_RESULT_TAG ||
       sys_result.scheduler_ready_mask!=0u ||
       sys_result.scheduler_pcb_type!=PORT_SYS_EXEC_PCB_KIND ||
       sys_result.scb_access_violation_pc_vector!=13u ||
       sys_result.scb_page_fault_pc_vector!=14u ||
       sys_result.system_time_low!=(uint32_t)native_now ||
       sys_result.system_time_high!=(uint32_t)(native_now>>32) ||
       sys_result.sysloa_linked!=1u ||
       sys_result.sysloa_idtr_base<0x10000u ||
       sys_result.sysloa_irq_ticks==0u ||
       sys_result.sysloa_home_lbn!=1u ||
       (sys_result.sysloa_cr0&0x80010001u)!=0x80010001u ||
       (uintptr_t)sys_result.resident_clock<PORT_SYS_ELF_START ||
       (uintptr_t)sys_result.resident_clock>
           PORT_SYS_ELF_LIMIT-sizeof(struct port_exe_systime) ||
       (uintptr_t)sys_result.pit_tick<PORT_SYS_ELF_START ||
       (uintptr_t)sys_result.pit_tick>=PORT_SYS_ELF_LIMIT ||
       (sys_result.resident_clock->sequence&1u)){
        bootstrap_message("INIT: SYS.EXE resident initialization failed.");
        bootstrap_halt();
    }
    bootstrap_message("INIT: loaded SYS.EXE resident entry executed and verified.");
    bootstrap_message("INIT: linked SYSLOA386 read real IDT, CR0, CR3, TSS, PIT and Files-11 boot home.");
    /* The original resident SYS.EXE owns system time after INIT, not the
     * earlier boot clock. Verify actual IRQ0 interrupts change resident bytes. */
    if(!arch_adopt_executive_clock(sys_result.resident_clock,
                                  sys_result.pit_tick) ||
       arch_exe_systime()!=sys_result.resident_clock){
        bootstrap_message("INIT: SYS.EXE PIT clock handoff failed.");
        bootstrap_halt();
    }
    uint32_t pit_before=arch_ticks();
    uint32_t sequence_before=sys_result.resident_clock->sequence;
    uint64_t time_before=((uint64_t)sys_result.resident_clock->high<<32)|
                                 sys_result.resident_clock->low;
    uint32_t spins=0;
    while((uint32_t)(arch_ticks()-pit_before)<3u && spins<30000000u){
        __asm__ volatile("pause");
        ++spins;
    }
    uint32_t pit_after=arch_ticks();
    uint32_t sequence_after=sys_result.resident_clock->sequence;
    uint64_t time_after=((uint64_t)sys_result.resident_clock->high<<32)|
                                sys_result.resident_clock->low;
    if((uint32_t)(pit_after-pit_before)<3u ||
       sequence_after-sequence_before<6u ||
       time_after-time_before<UINT64_C(300000)){
        bootstrap_message("INIT: live SYS.EXE PIT clock did not advance.");
        bootstrap_halt();
    }
    bootstrap_message("INIT: actual IRQ0 advanced resident SYS.EXE VMS clock.");

    static struct port_sys_exec_result dq_publish_result;
    const struct port_sys_exec_start dq_publish_request={
        .signature=PORT_SYS_EXEC_SIGNATURE,.version=PORT_SYS_EXEC_VERSION,
        .pcb_type=PORT_SYS_EXEC_PCB_KIND,.result=&dq_publish_result,
        .phase=PORT_SYS_EXEC_INIT_DQ_DDT_PUBLISH,.dq_boot_io=&dq_boot_io};
    if(loaded_sys(&dq_publish_request)!=PORT_SYS_EXEC_NORMAL||
       dq_publish_result.signature!=PORT_SYS_EXEC_DQ_DDT_PUBLISH_TAG){
        bootstrap_message("INIT: DQA0 DDT publication phase rejected its source prerequisites.");
        bootstrap_halt();
    }
    if(dq_publish_result.dqa_ioc_status==PORT_DQDDT_BIND_BAD){
        bootstrap_message("INIT: DQA0 DDT native pointer/CRB publication validation failed.");
        bootstrap_halt();
    }
    if(dq_publish_result.dqa_ioc_status!=PORT_V37_IOC_OK){
        bootstrap_message("INIT: IOC$SEARCHDEV did not find published DQA0.");
        bootstrap_halt();
    }
    if(dq_publish_result.dq_ddt_published!=1u||
       !dq_publish_result.device_db||!dq_publish_result.dq_native_ddt||
       !dq_publish_result.ioc_search||
       dq_publish_result.dq_published_f11_read_bytes!=700u||
       dq_publish_result.dq_published_f11_read_irps!=2u||
       !dq_publish_result.dq_acp_header||
       (uintptr_t)dq_publish_result.dq_acp_header<PORT_SYS_ELF_START||
       (uintptr_t)dq_publish_result.dq_acp_header>
          PORT_SYS_ELF_LIMIT-512u||
       dq_publish_result.dq_acp_header_lbn==0u||
       dq_publish_result.dq_acp_header_reads!=1u||
       dq_publish_result.dq_acp_file!=11u||
       dq_publish_result.dq_acp_sequence!=11u||
       !dq_publish_result.dq_acp_fcb||
       (uintptr_t)dq_publish_result.dq_acp_fcb<PORT_SYS_ELF_START||
       (uintptr_t)dq_publish_result.dq_acp_fcb>
          PORT_SYS_ELF_LIMIT-sizeof(struct port_f11_87_fcb)||
       dq_publish_result.dq_acp_file_blocks==0u||
       dq_publish_result.dqa_ioc_ddb!=dq_publish_result.device_db->ddb[0]||
       dq_publish_result.dqa_ioc_ucb!=dq_publish_result.device_db->ucb[0]||
       dq_publish_result.device_db->incomplete_driver_mask!=6u||
       dq_publish_result.device_db->bound_dqa_ddt!=
          dq_publish_result.dq_native_ddt->base){
        bootstrap_message("INIT: DQA0 native DDT IOC publication or SEARCHDEV failed.");
        bootstrap_halt();
    }
    bootstrap_message("INIT: SYS.EXE published native DQA0 DDT in DDB/UCB; IOC$SEARCHDEV found bound DQA0 (partial driver).");
    bootstrap_message("INIT: Files-11 FH2 chain read 700 SYS.EXE bytes across two sectors through IOC-selected DQA0 DDT/IRQ14.");
    bootstrap_message("INIT: Files-11 ACP RDHEDR/F11B allocated SYS.EXE FID 11 FCB from native EXE$ALONONPAGED pool via DQA0 DDT/CRB IRQ14.");

    int ata_write_probe=m202_ata_write_probe(dq_ddt_result.dq_ddt_stage,
        dq_result.dq_source_fdt,dq_publish_result.device_db_bytes,
        dq_publish_result.device_db,dq_publish_result.dq_native_ddt,
        dq_publish_result.ioc_search);
    if(ata_write_probe<0){
        bootstrap_message("INIT: marked QEMU ATA PIO write/readback/restore regression failed.");
        bootstrap_halt();
    }
    if(ata_write_probe>0)
        bootstrap_message("INIT: SYS.EXE DQA0 write FIFO completed two ordered DDT WRITEPBLK IRPs; read FIFO posted two READPBLK IRPs via IRQ14, readback and restore verified.");

    /* INIT.MAR's remaining SYSLOA, SYSPARAM, SPT, ACP and process-image
     * prerequisites have not been ported. Stop before login or DCL. */
    bootstrap_message("INIT stopped: remaining original initialization dependencies incomplete.");
    bootstrap_halt();


}
