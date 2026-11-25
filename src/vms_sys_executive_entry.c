#include "vms_sys_executive_entry.h"
#include "vms_exe_sched.h"
#include "vms_exe_scbvector.h"
#include "vms_exe_gettim.h"
#include "vms_sysloavec.h"
#include "vms_ini_log_templates.h"
#include "vms_ini_sysparams.h"
#include "vms_ini_dpt_templates.h"
#include "vms_ini_syslog.h"
#include "vms_ini_devicedb.h"
#include "vms_ini_pc_bus.h"
#include "vms_con_port.h"
#include "vms_ioc_phys.h"
#include "vms_ioc_mapvblk.h"
#include "vms_devlog_search.h"
#include "vms_dqddt.h"
#include "vms_dq_ioc.h"
#include "vms_relocdrv.h"
#include "vms_ddtreloc.h"
#include "vms_dq_index_header.h"
#include "vms_dq_mfd_header.h"
#include "vms_dq_dir.h"
#include "vms_dq_imageprobe.h"
#include "vms_dq_aux_images.h"
#include "vms_f11_range.h"
#include "vms_f11_bootpath.h"
#include "vms_f11_bootdir.h"
#include "vms_f11_chain.h"
#include "vms_f11_acp87.h"
#include "vms_f11_cursor.h"
#include "vms_dq_ioc.h"
#include "vms_f11_ioc.h"
#include "vms_f11_channel.h"
#include "vms_qio_post.h"
#include "vms_qio_irp_pool.h"
#include "vms_exe_np_pool.h"
#include "vms_nl_ddt.h"
#include "vms_iopost_end.h"
#include "vms_sysboot_elf.h"
#include <stdint.h>
/* All state belongs to SYS.EXE's separately loaded resident supervisor
 * segment, not to the SYSBOOT monitor or a pretend VAX processor. */
static struct port_exe_scheduler executive_scheduler;
static struct port_exe_scb_slot executive_scb[PORT_SCB_ENTRIES];
static struct port_exe_systime executive_clock;
/* Interrupts advance the actual resident image clock after source-ordered INIT. */
static void executive_pit_clock_tick(void){
    port_exe_systime_tick(&executive_clock);
}
static struct port_sysloa_table executive_sysloa;
/* This memory is genuinely loaded and zeroed by the SYSBOOT ELF loader,
 * mapped supervisor-only in the guest. Replacing its provisional size
 * limits with SYSGEN-selected BOO$ offsets remains later integration. */
static uint8_t executive_packet_pages[96u*1024u] __attribute__((aligned(512)));
static struct port_ini_packet_list executive_irps,executive_lrps,executive_srps;
static uint32_t executive_packet_ready;
/* INIT's later nonpaged records occupy only the free low end of the
 * actual SYS.EXE supervisor RAM; no boot monitor array stands in for it. */
static uint32_t executive_free_npages;
static struct port_exe_np_pool executive_np_pool;
static uint32_t executive_np_pool_ready;
static uint32_t executive_acp_allocation_source=UINT32_MAX;

/* The single-CPU IA-32 executive maps source DSBINT/ENBINT synchronization
 * to preserving and restoring the caller's interrupt-enable state. */
#if !defined(__i386__)
static uint32_t executive_qio_host_sync_depth;
#endif
static uint32_t executive_qio_raise_sync(void *unused){
    (void)unused;
#if defined(__i386__)
    uint32_t flags;
    __asm__ volatile("pushfl; popl %0; cli":"=r"(flags)::"memory");
    return flags;
#else
    uint32_t old=executive_qio_host_sync_depth;
    ++executive_qio_host_sync_depth;
    return old;
#endif
}
static void executive_qio_restore_ipl(void *unused,uint32_t flags){
    (void)unused;
#if defined(__i386__)
    if(flags&(1u<<9))__asm__ volatile("sti":::"memory");
#else
    executive_qio_host_sync_depth=flags;
#endif
}
static const struct port_v37_irp_pool_sync executive_qio_pool_sync={
    0,executive_qio_raise_sync,executive_qio_restore_ipl};
static uint32_t executive_qio_alloc_irp(uint8_t **packet,uint32_t *va){
    return port_v37_irp_pool_alloc(&executive_irps,PORT_V37_IRP_SIZE,
        &executive_qio_pool_sync,packet,va);
}
static uint32_t executive_qio_free_irp(uint8_t *packet,uint32_t va){
    if(!packet)return PORT_V37_IRP_POOL_BAD;
    return port_v37_irp_pool_free(&executive_irps,packet,va,
        port_v37_read16(packet+8u),&executive_qio_pool_sync);
}
static struct port_ini_lockdb executive_lockdb;
static uint32_t executive_lockdb_ready;
static struct port_ini_procvec executive_procvec;
static uint32_t executive_procvec_ready;
static struct port_ini_phvswap executive_phvswap;
static uint32_t executive_phvswap_ready;
static struct port_ini_mpw executive_mpw;
static uint32_t executive_mpw_ready;
static struct port_ini_pflvec executive_pflvec;
static uint32_t executive_pflvec_ready;
/* Original MMG$GL_NULLPFL is a resident symbol in SYS.EXE. This native
 * storage is an address anchor only; its unported PFL fields and paging
 * consumers must not be treated as initialized. */
static uint32_t executive_nullpfl_uninitialized[16];
static struct port_ini_interrupt_stack executive_interrupt_stack;
static uint32_t executive_interrupt_stack_ready;
/* INIT overlays BOO$L_SYS_MAP with the retrieval-pointer front in place.
 * Persistent result lives in SYS.EXE; BOO$ storage remains SYSBOOT-owned.
 * Header offsets are temporary native metadata; WCB consumers are blocked. */
static uint8_t executive_wcb_header[48] __attribute__((aligned(4)));
static struct port_ini_syswcb executive_system_wcb;
static uint32_t executive_system_wcb_ready;
/* Original INIT.MAR 328-358, 1240-1255; typed LOG records occupy
 * resident SYS.EXE nonpaged data. Live table insertion still requires
 * original SYSLOGNAM, boot DDB/UCB and SWAPINIT translation. */
static uint8_t executive_disklog[PORT_INI_LOG_TEMPLATE_BYTES];
static uint8_t executive_sysdevlog[PORT_INI_LOG_TEMPLATE_BYTES];
static struct port_ini_syslog_result executive_syslog;
static uint32_t executive_syslog_ready;
/* Source DPTAB prefix bytes live in the loaded native SYS.EXE. All
 * lengths are intentionally invalid for IOC DPT-list insertion; the
 * NLA DDT will bind separately after its native FDT compiles. */
static uint8_t executive_dpt_stages[3][PORT_V37_DPT_STAGE_BYTES] __attribute__((aligned(4)));
static uint32_t executive_dpt_stages_ready;
static uint32_t executive_null_fdt_ready;
/* Source DQDRIVER legal, buffered and ten class masks. No read/write HW. */
static struct port_dq_fdt executive_dq_fdt __attribute__((aligned(8)));
static uint32_t executive_dq_fdt_ready;
/* DQDRIVER physical READPBLK data really comes from the registered
 * native PC ATA PIO driver. It cannot be issued as a process QIO yet. */
static union {uint32_t align;uint8_t bytes[PORT_V37_IRP_SIZE];} executive_dq_boot_irp;
static union {uint32_t align;uint16_t words[256];} executive_dq_boot_sector;
static struct port_dq_bootread_result executive_dq_bootread_result;
/* The original DQ PREPROCESS/FUNCXT path can address any physical LBN.
 * Exercise the native PC ATA adapter at LBN0 in a distinct IRP, with
 * no CCB, process QIO, DDT publication, or dirty user buffers. */
static union {uint32_t align;uint8_t bytes[PORT_V37_IRP_SIZE];} executive_dq_bootzero_irp;
static union {uint32_t align;uint16_t words[256];} executive_dq_bootzero_sector;
static struct port_dq_bootread_result executive_dq_bootzero_result;
static uint32_t executive_dq_bootzero_ready;
/* FIL$MOUNT root INDEXF.SYS FH2, physically read in its own resident IRP.
 * The DQ device dispatch and Files-11 ACP are still deliberately unbound. */
static union {uint32_t align;uint8_t bytes[PORT_V37_IRP_SIZE];} executive_dq_index_irp;
static union {uint32_t align;uint16_t words[256];} executive_dq_index_sector;
static struct port_dq_bootread_result executive_dq_index_io;
static struct port_dq_index_result executive_dq_index_result;
static uint32_t executive_dq_index_ready;
/* FID$C_MFD is resolved through real INDEXF.SYS extent bytes. */
static union {uint32_t align;uint8_t bytes[PORT_V37_IRP_SIZE];} executive_dq_mfd_irp;
static union {uint32_t align;uint16_t words[256];} executive_dq_mfd_sector;
static struct port_dq_bootread_result executive_dq_mfd_io;
static struct port_dq_mfd_result executive_dq_mfd_result;
static uint32_t executive_dq_mfd_ready;
/* Three additional genuine ATA IRPs: MFD data, SYSEXE.DIR header,
 * SYSEXE.DIR data. No public IOC channel, ACP, or driver publication. */
static union {uint32_t align;uint8_t bytes[3u*PORT_V37_IRP_SIZE];} executive_dq_dir_irps;
static union {uint32_t align;uint16_t words[256];} executive_dq_dir_mfd_data;
static union {uint32_t align;uint16_t words[256];