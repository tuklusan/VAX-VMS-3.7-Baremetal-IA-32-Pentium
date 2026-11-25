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
static union {uint32_t align;uint16_t words[256];} executive_dq_dir_sysexe_header;
static union {uint32_t align;uint16_t words[256];} executive_dq_dir_sysexe_data;
static struct port_dq_bootread_result executive_dq_dir_io[3];
static struct port_dq_dir_fid executive_sysexe_dir_fid;
static struct port_dq_dir_fid executive_sysexecutable_fid;
static uint32_t executive_dq_dir_ready;
/* Native SYS.EXE FH2 and first ELF block through real ATA read-only IO.
 * The original VAX image activator and Files-11 ACP remain unlinked. */
static union {uint32_t align;uint8_t bytes[2u*PORT_V37_IRP_SIZE];} executive_dq_image_irps;
static union {uint32_t align;uint16_t words[256];} executive_dq_image_header;
static union {uint32_t align;uint16_t words[256];} executive_dq_image_data;
static struct port_dq_bootread_result executive_dq_image_io[2];
static struct port_dq_image_result executive_dq_image_probe;
/* M161-M168: source DQ_STARTIO/IOCIOPOST request queue is private.
 * Its actual physical callback remains the privileged ATA IRP adapter;
 * no public IOC DQA0 unit is published by this stage. */
static struct port_dq_ioc_queue executive_dq_private_ioc;
/* Separate read-only Files-11 -> private IOC bridge. The IOC packet
 * buffer is resident, not allocated on the small bootstrap stack. */
static struct port_dq_ioc_queue executive_f11_ioc_queue;
static struct port_f11_ioc_file executive_f11_ioc_file;
/* M185-M192 read-only owner-scoped private handle table; DQA0 remains unbound. */
static struct port_dq_ioc_queue executive_f11_channel_ioc_queue;
static struct port_f11_chan_table executive_f11_channel_table;
static uint8_t executive_ioc_private_sectors[3][512] __attribute__((aligned(4)));

static uint32_t executive_dq_image_ready;
/* Tenth read checks disk text against bytes already resident in SYS.EXE. */
static union {uint32_t align;uint8_t bytes[PORT_V37_IRP_SIZE];} executive_dq_text_irp;
static union {uint32_t align;uint16_t words[256];} executive_dq_text_sector;
static struct port_dq_bootread_result executive_dq_text_io;
/* The separately preloaded SYSLOA386/INILOA image text ranges are
 * validated by real disk I/O, not a copied disk fixture or a second loader. */
static union {uint32_t align;uint8_t bytes[PORT_V37_IRP_SIZE];} executive_dq_aux_irp;
static union {uint32_t align;uint16_t words[256];} executive_dq_aux_sector;
static uint8_t executive_dq_aux_header[512] __attribute__((aligned(4)));
static uint8_t executive_dq_aux_elf[512] __attribute__((aligned(4)));
static struct port_dq_bootread_result executive_dq_aux_io;
static uint32_t executive_dq_aux_bytes;
static uint32_t executive_dq_aux_count;
/* Guest-side bounded FIL$READVBN byte stream. The read callback uses
 * the registered privileged physical DQ IRP and its real UCB counter. */
static union {uint32_t align;uint8_t bytes[PORT_V37_IRP_SIZE];} executive_f11_range_irp;
static union {uint32_t align;uint16_t words[256];} executive_f11_range_sector;
static uint8_t executive_f11_range_result[1024] __attribute__((aligned(4)));
static struct port_dq_bootread_result executive_f11_range_io;
struct executive_f11_context {
    struct port_dq_bootread_request native;
    uint32_t previous_count,reads,ucb;
};
static uint32_t executive_f11_native_sector(void *opaque,uint32_t lbn,
                                             uint8_t out[512]){
    struct executive_f11_context *ctx=opaque;
    if(!ctx||!out)return PORT_DQ_DIR_MAP;
    uint8_t *irp=executive_f11_range_irp.bytes;
    if(port_v37_irp_prepare(irp,PORT_V37_IRP_SIZE,
         (uint32_t)(uintptr_t)irp,
         ctx->ucb,PORT_DQ_READPBLK)!=PORT_V37_QIO_OK){
        executive_f11_range_io.ucb_io_count=UINT32_C(0xe001);
        return PORT_DQ_DIR_MAP;
    }
    port_v37_write16(irp+42u,PORT_DQ_PHYSIO);
    port_v37_write16(irp+50u,PORT_DQ_BOOTREAD_BYTES);
    port_v37_write32(irp+PORT_V37_IRP_MEDIA,lbn);
    ctx->native.irp=irp;
    ctx->native.sector=executive_f11_range_sector.words;
    uint32_t read_status=port_dq_bootread_execute_physical(&ctx->native,
             &executive_f11_range_io,lbn);
    if(read_status!=PORT_DQ_BOOTREAD_OK){
        executive_f11_range_io.ucb_io_count=UINT32_C(0xe000)|read_status;
        return PORT_DQ_DIR_MAP;
    }
    if(executive_f11_range_io.ucb_io_count!=ctx->previous_count+ctx->reads+1u){
        executive_f11_range_io.ucb_io_count=UINT32_C(0xd000)|
                 (executive_f11_range_io.ucb_io_count&0x0fffu);
        return PORT_DQ_DIR_MAP;
    }
    for(unsigned i=0u;i<512u;i++)
        out[i]=((const uint8_t *)executive_f11_range_sector.words)[i];
    ctx->reads++;
    return PORT_DQ_DIR_OK;
}

/* Read-only native Files-11 private path, wholly in resident SYS.EXE.
 * The full DQA0 DDT and ACP are still absent. */
static struct port_f11_boot_file executive_f11_opened_sys;
static uint8_t executive_f11_opened_bytes[768] __attribute__((aligned(4)));
static uint32_t executive_f11_bootpath_ready;
/* Original FIL$RDCHKFILHDR extension FID scan, validated from actual
 * native disk; original full RMS/ACP channel handling is still absent. */
static struct port_f11_chain executive_f11_chain;
static uint8_t executive_f11_chain_text[777] __attribute__((aligned(4)));
struct executive_dir_listing {uint32_t mask,entries;};
static uint32_t executive_source_directory_entry(void *opaque,
    const uint8_t *name,size_t n,uint16_t version,
    const struct port_dq_dir_fid *fid){
    struct executive_dir_listing *list=opaque;
    if(!list||!name||!fid||!version)return 0u;
    list->entries++;
    static const struct {char name[16];uint8_t length;uint16_t fid;}
        images[3]={{"SYS.EXE",7u,11u},{"INILOA.EXE",10u,12u},
                   {"SYSLOA386.EXE",13u,13u}};
    for(unsigned j=0;j<3u;j++){
        if(n!=images[j].length)continue;
        unsigned same=1u;
        for(unsigned i=0;i<n;i++)if(name[i]!=(uint8_t)images[j].name[i])same=0u;
        if(same){
            if(version!=1u||fid->number!=images[j].fid||
               fid->sequence!=images[j].fid||fid->volume!=0u||
               (list->mask&(1u<<j)))return 0u;
            list->mask|=1u<<j;
        }
    }
    return 1u;
}

static uint32_t executive_dq_bootread_ready;
/* Source DQDRIVER DDTAB and FUNCTAB are inactive until original IOC/ACP. */
static struct port_dqddt_stage executive_dqddt;
static uint32_t executive_dqddt_ready;
static struct port_dq_crb executive_dq_crb;
static uint32_t executive_dq_crb_read_ready;
static struct port_dq_ioc_queue executive_dq_native_ioc;
struct executive_dq_ioc_context {
    port_dq_native_read_fn read_sector;
    void *read_context;
};
static uint32_t executive_dq_ioc_sector(void *opaque,uint32_t lbn,
                                         uint8_t destination[512]){
    struct executive_dq_ioc_context *ctx=opaque;
    if(!ctx||!ctx->read_sector||!destination)return 0u;
    uint16_t sector[256];
    if(ctx->read_sector(ctx->read_context,lbn,sector)!=0)return 0u;
    for(unsigned i=0;i<256u;i++){
        destination[i*2u]=(uint8_t)sector[i];
        destination[i*2u+1u]=(uint8_t)(sector[i]>>8u);
    }
    return 1u;
}
static int executive_dq_ioc_read_sector(void *opaque,uint32_t lbn,
                                         uint16_t destination[256]){
    const struct port_sys_exec_dq_boot_settings *settings=opaque;
    if(!settings||!settings->read_sector||!destination)return -1;
    struct executive_dq_ioc_context ctx={settings->read_sector,
                                         settings->context};
    struct port_dq_ioc_report report;
    if(port_dq_ioc_init(&executive_dq_native_ioc,
           executive_dq_ioc_sector,&ctx)!=PORT_DQ_IOC_OK||
       port_dq_ioc_prepare(&executive_dq_native_ioc,1u,lbn,
           (uint8_t *)destination,512u)!=
           PORT_DQ_IOC_OK||
       port_dq_ioc_enqueue(&executive_dq_native_ioc,1u)!=PORT_DQ_IOC_OK||
       port_dq_ioc_run(&executive_dq_native_ioc,&report)!=PORT_DQ_IOC_OK||
       report.processed!=1u||report.succeeded!=1u||report.reads!=1u)
        return -1;
    return 0;
}
/* Exercise the staged READPBLK bridge with a resident IRP. The actual
 * private DQ IOC FIFO reaches ATA through the attached native CRB. DDB DDT
 * actions and public process QIO remain unavailable. */
static union {uint32_t align;uint16_t words[256];} executive_dqddt_sector;
static struct port_dq_bootread_result executive_dqddt_io;
/* Source RELOCDRV.MAR INITDRV DQ DPT STORE runs only on private boot
 * copies until real DQ driver has CRB and interrupts. */
static struct port_relocdrv_dq executive_relocdrv_dq;
static uint32_t executive_relocdrv_dq_ready;
/* IOC$RELOC_DDT runs only on this resident scratch copy. Source DQ's
 * the single marked READPBLK slot remains a private native boot action. */
static union {uint32_t align; uint8_t bytes[PORT_DQDDT_IMAGE_BYTES];}
    executive_relocated_private;
static uint32_t executive_relocated_private_ready;
static struct port_dqddt_native executive_native_dqddt;
static uint32_t executive_private_reloc_check(const uint8_t *b,size_t n,uint32_t base){
    if(!b||b!=executive_relocated_private.bytes||
       n!=PORT_DQDDT_IMAGE_BYTES||!executive_relocated_private_ready||
       base!=(uint32_t)(uintptr_t)b||
       port_ddtreloc_read32(b+8u)!=base+PORT_DQDDT_HEADER_BYTES||
       port_ddtreloc_read16(b+36u)!=PORT_DQDDT_FDT_BYTES||
       executive_dqddt.driver_bound||executive_relocdrv_dq.driver_bound)
        return PORT_DDTRELOC_BAD;
    for(unsigned i=0;i<PORT_DQDDT_ACTIONS;i++){
        const unsigned offset=PORT_DQDDT_HEADER_BYTES+24u+12u*i;
        const unsigned row=i+2u;
        const uint32_t source_action=row==PORT_DQ_FDT_READ?
            (executive_dqddt.native_read_action_pc|UINT32_C(0x80000000)):
            row==PORT_DQ_FDT_WRITE?
            (executive_dqddt.native_write_action_pc|UINT32_C(0x80000000)):0u;
        const uint32_t expected=row==PORT_DQ_FDT_READ||
            row==PORT_DQ_FDT_WRITE?source_action:base+offset-8u;
        if(port_ddtreloc_read32(executive_dqddt.image+offset)!=source_action||
           port_ddtreloc_read32(b+offset)!=expected)
            return PORT_DDTRELOC_BAD;
    }
    return PORT_DDTRELOC_OK;
}

/* FIN_DATABASE follows a real physical boot-disk home-sector read. These
 * remain nonpublished LOG records, and the DQ DDT remains absent. */
static struct port_ini_fin_result executive_fin_database_result;
static uint32_t executive_fin_database_ready;
/* LOGNAMSUB private native source SYSTEM boot-name table only. */
static struct port_log_system_table executive_boot_system_log;
static uint32_t executive_boot_system_log_ready;
static struct port_ini_bootdev_result executive_bootdev_scan;
static uint32_t executive_bootdev_scan_ready;
/* DEVICEDAT.MAR prelinked BOOT/OPA/MBA/NLA DDB and UCB backing. */
static union { uint32_t align; uint8_t data[PORT_V37_DEVICE_BYTES]; } executive_devicedb_bytes;
static struct port_v37_device_result executive_devicedb_result;
static uint32_t executive_devicedb_ready;

struct executive_postpub_f11_context {
    const struct port_sys_exec_dq_boot_settings *settings;
    uint32_t reads;
};
struct executive_acp_io_context {
    struct executive_postpub_f11_context disk;
    const uint8_t *index_header;
    uint16_t volume;
};
static void *executive_acp_alloc(void *opaque,uint32_t bytes,uint32_t tag){
    (void)opaque;
    const uint32_t minimum=tag==PORT_F11_87_FCBTYPE?
        sizeof(struct port_f11_87_fcb):sizeof(struct port_f11_87_window);
    if(!executive_np_pool_ready||
       (tag!=PORT_F11_87_FCBTYPE&&tag!=PORT_F11_87_WCBTYPE)||
       bytes<minimum)
        return 0;
    struct port_exe_np_allocator allocator={
        &executive_irps,&executive_lrps,&executive_srps,
        &executive_np_pool,&executive_qio_pool_sync};
    struct port_exe_np_allocation allocation={0};
    if(port_exe_np_allocate(&allocator,bytes,(uint8_t)tag,&allocation)!=
          PORT_EXE_NP_POOL_OK||
       !allocation.address||allocation.bytes<minimum)
        return 0;
    executive_acp_allocation_source=allocation.source;
    return allocation.address;
}
static uint32_t executive_acp_dealloc(void *opaque,void *memory,
                                      uint32_t bytes,uint32_t tag){
    (void)opaque;
    if(!executive_np_pool_ready||
       (tag!=PORT_F11_87_FCBTYPE&&tag!=PORT_F11_87_WCBTYPE))
        return PORT_EXE_NP_POOL_BAD;
    struct port_exe_np_allocator allocator={
        &executive_irps,&executive_lrps,&executive_srps,
        &executive_np_pool,&executive_qio_pool_sync};
    return port_exe_np_deallocate(&allocator,memory,bytes,(uint8_t)tag);
}
static uint32_t executive_postpub_f11_sector(void *,uint32_t,uint8_t[512]);
static uint32_t executive_acp_map_vbn(void *opaque,uint32_t vbn,
                                      uint32_t *lbn){
    struct executive_acp_io_context *ctx=opaque;
    if(!ctx||!ctx->index_header||!lbn)return PORT_F11_87_BADPARAM;
    return port_dq_dir_map_vbn(ctx->index_header,vbn,lbn)==PORT_DQ_DIR_OK?
        PORT_F11_87_NORMAL:PORT_F11_87_BADFILEHDR;
}
static uint32_t executive_acp_read_lbn(void *opaque,uint32_t lbn,
                                       uint8_t output[512]){
    struct executive_acp_io_context *ctx=opaque;
    if(!ctx||!output)return PORT_F11_87_BADPARAM;
    return executive_postpub_f11_sector(&ctx->disk,lbn,output)==PORT_DQ_DIR_OK?
        PORT_F11_87_NORMAL:PORT_F11_86_IOERROR;
}
static uint32_t executive_acp_switch_volume(void *opaque,uint16_t volume){
    const struct executive_acp_io_context *ctx=opaque;
    return ctx&&ctx->volume==volume?PORT_F11_87_NORMAL:
        PORT_F11_87_WRONGACP;
}
static uint32_t executive_postpub_f11_sector(void *opaque,uint32_t lbn,
                                              uint8_t output[512]){
    struct executive_postpub_f11_context *ctx=opaque;
    if(!ctx||!ctx->settings||!ctx->settings->read_sector||!output||
       ctx->settings->boot_unit!=0u||ctx->settings->reserved||
       !ctx->settings->irq_handler)return PORT_DQ_DIR_MAP;
    struct port_v37_ioc_result selected={0};
    if(port_v37_ioc_searchdev(executive_devicedb_bytes.data,
         sizeof executive_devicedb_bytes.data,
         (uint32_t)(uintptr_t)executive_devicedb_bytes.data,
         &executive_devicedb_result,"DQA0",4u,&selected)!=PORT_V37_IOC_OK||
       selected.ddb!=executive_devicedb_result.ddb[0]||
       selected.ucb!=executive_devicedb_result.ucb[0]||
       selected.unit!=0u||!selected.driver_bound)
        return PORT_DQ_DIR_MAP;
    const uint8_t *ddb=executive_devicedb_bytes.data+
        PORT_V37_DDB_BOOT_OFF;
    const uint32_t ddt=port_v37_read32(ddb+12u);
    if(ddt!=executive_native_dqddt.base||
       port_v37_read32((const uint8_t *)(uintptr_t)ddt)!=
           (uint32_t)(uintptr_t)port_dqddt_startio)
        return PORT_DQ_DIR_MAP;
    typedef uint32_t (*dq_startio_fn)(const struct port_dqddt_stage *,
        const struct port_dqddt_native *,const struct port_dq_fdt *,
        const struct port_dq_bootread_request *,struct port_dq_bootread_result *);
    dq_startio_fn startio=(dq_startio_fn)(uintptr_t)
        port_v37_read32((const uint8_t *)(uintptr_t)ddt);
    const uint32_t irp_free_before=executive_irps.free_count;
    uint8_t *irp=0;uint32_t irp_va=0u;
    if(executive_qio_alloc_irp(&irp,&irp_va)!=PORT_V37_IRP_POOL_OK)
        return PORT_DQ_DIR_MAP;
    if(port_v37_irp_prepare(irp,PORT_V37_IRP_SIZE,irp_va,selected.ucb,
            PORT_DQ_READPBLK)!=PORT_V37_QIO_OK){
        (void)executive_qio_free_irp(irp,irp_va);
        return PORT_DQ_DIR_MAP;
    }
    port_v37_write16(irp+42u,PORT_DQ_PHYSIO);
    port_v37_write16(irp+50u,PORT_DQ_BOOTREAD_BYTES);
    port_v37_write32(irp+PORT_V37_IRP_MEDIA,lbn);
    const struct port_dq_bootread_request request={
        .fdt=&executive_dq_fdt,
        .device_db=executive_devicedb_bytes.data,
        .device_info=&executive_devicedb_result,
        .irp=irp,.irp_bytes=PORT_V37_IRP_SIZE,
        .sector=executive_dqddt_sector.words,
        .sector_bytes=PORT_DQ_BOOTREAD_BYTES,.boot_unit=0u,
        .read_sector=ctx->settings->read_sector,
        .read_context=ctx->settings->context};
    const uint32_t read_status=startio(&executive_dqddt,
         &executive_native_dqddt,&executive_dq_fdt,&request,
         &executive_dqddt_io);
    const uint32_t free_status=executive_qio_free_irp(irp,irp_va);
    if(read_status!=PORT_DQ_BOOTREAD_OK||
       free_status!=PORT_V37_IRP_POOL_OK||
       executive_irps.free_count!=irp_free_before||
       executive_dqddt_io.io_function!=PORT_DQ_READPBLK||
       executive_dqddt_io.lbn!=lbn||
       executive_dqddt_io.irp_status!=PORT_SYS85_NORMAL||
       executive_dqddt_io.bytes!=PORT_DQ_BOOTREAD_BYTES)
        return PORT_DQ_DIR_MAP;
    for(unsigned i=0u;i<512u;i++)
        output[i]=((const uint8_t *)executive_dqddt_sector.words)[i];
    ctx->reads++;
    return PORT_DQ_DIR_OK;
}
/* Full native NL read/write DDT/FDT bytes; class/port drivers still absent. */
static union {uint32_t align;uint8_t bytes[PORT_V37_NL_IMAGE_BYTES];} executive_nl_ddt;
static uint32_t executive_nl_ddt_ready;
static union {uint32_t align;uint8_t bytes[PORT_V37_OPA_UCB_SIZE];} executive_con_scratch;
static uint32_t executive_conport_ready;
static struct port_v37_ioc_result executive_ioc_samples[3];
static uint32_t executive_ioc_ready;
static struct port_devlog_trace executive_device_log_samples[3];
static uint32_t executive_ioc_logical_ready;
/* Actual SYSDEF.MDL source IRP packet bytes in resident SYS.EXE. The
 * disabled lookaside allocator cannot yet supply IOC packets safely, so
 * these are two reserved bootstrap IRPs, never user-visible channels. */
static union {uint32_t align;uint8_t bytes[2u*PORT_V37_IRP_SIZE];} executive_post_irps;
static struct port_v37_iopost_queue executive_post_queue;
static uint32_t executive_post_ready;
static uint32_t executive_post_soft_count,executive_post_soft_level;
static struct port_v37_iopost_endlog executive_iopost_completed;
static uint32_t executive_iopost_end_ready;
static uint32_t executive_iopost_irq_armed;
/* Source IOCIOPOST.MAR kernel end-action REMQUE loop, now called by a
 * real CPL0-only x86 software INT 0x90 interrupt gate. */
static uint32_t executive_iopost_irq_entry(void){
    const uintptr_t native_end=(uintptr_t)port_v37_iopost_kernel_end;
    if(!executive_iopost_irq_armed||executive_iopost_end_ready||
       !executive_post_ready||executive_post_queue.queued!=2u||
       native_end<PORT_SYS_ELF_START||native_end>=PORT_SYS_ELF_LIMIT)
        return PORT_V37_IOPOST_BAD;
    /* Prevent an accidental nested entry from draining freed IRP links. */
    executive_iopost_irq_armed=0u;
    if(port_v37_iopost_drain(&executive_post_queue,PORT_SYS_ELF_START,
          PORT_SYS_ELF_LIMIT,(uint32_t)native_end,&executive_iopost_completed,2u)
          !=PORT_V37_IOPOST_OK)
        return PORT_V37_IOPOST_CORRUPT;
    executive_iopost_end_ready=1u;
    return PORT_V37_IOPOST_OK;
}
static void executive_post_mark_softint(void *unused,uint8_t ipl){
    (void)unused;
    executive_post_soft_count++;
    executive_post_soft_level=ipl;
}

/* Original SYSTEM and NULL PCB slots have distinct persistent resident
 * storage. Creating these source-derived scheduler descriptors is not a
 * substitute for actually scheduling SYSTEM or running LOGINOUT. */
static struct port_exe_sched_process executive_system_pcb;
static struct port_exe_sched_process executive_null_pcb;
_Alignas(8) static uint8_t executive_vms_system_pcb[PORT_V37_PCB_BYTES];
_Alignas(8) static uint8_t executive_vms_null_pcb[PORT_V37_PCB_BYTES];
_Alignas(8) static uint8_t executive_vms_swapper_pcb[PORT_V37_PCB_BYTES];
static struct port_v37_scheduler executive_vms_scheduler;
static uint32_t executive_vms_address(void *unused,const void *p){
    (void)unused;
    return (uintptr_t)p<=UINT32_MAX?(uint32_t)(uintptr_t)p:0u;
}
static uint8_t *executive_vms_resolve(void *unused,uint32_t a,size_t n){
    (void)unused;
    const uintptr_t start=(uintptr_t)&executive_vms_scheduler.heads[0];
    const uintptr_t limit=(uintptr_t)&executive_vms_scheduler.heads[64];
    if(a>=start&&a<=limit&&n<=limit-a)return (uint8_t *)(uintptr_t)a;
    const uintptr_t pcbs[3]={(uintptr_t)executive_vms_null_pcb,
        (uintptr_t)executive_vms_swapper_pcb,
        (uintptr_t)executive_vms_system_pcb};
    for(unsigned i=0;i<3u;i++)
        if(a>=pcbs[i]&&a<=pcbs[i]+PORT_V37_PCB_BYTES&&
           n<=pcbs[i]+PORT_V37_PCB_BYTES-a)return (uint8_t *)(uintptr_t)a;
    return 0;
}



static uint32_t port_exe_initialize_lookaside(const struct port_sys_exec_start *p){
    /* INIT.MAR INI_IRP/INI_LRP/INI_SRP must finish before LINK_VEC. The
     * BOOTS/SRC/SYSBOOT.MAR allocation calculation defines these spans. */
    uintptr_t start=(uintptr_t)executive_packet_pages;
    uintptr_t top=start+sizeof executive_packet_pages;
    if(start>UINT32_MAX || top>UINT32_MAX ||
       !p->packet_settings || executive_packet_ready)
        return PORT_SYS_EXEC_BAD_PARAMETERS;
    struct port_boo_packet_settings settings=*p->packet_settings;
    settings.top_address=(uint32_t)top;
    /* A future SYSBOOT page-table mapper must support virtual-count gaps;
     * this restricted stage has only the physically backed resident pages. */
    if(settings.irp_count!=settings.irp_virtual_count ||
       settings.lrp_count!=settings.lrp_virtual_count ||
       settings.srp_count!=settings.srp_virtual_count ||
       settings.nonpaged_dynamic_bytes || settings.nonpaged_virtual_bytes ||
       settings.paged_dynamic_bytes)
        return PORT_SYS_EXEC_BAD_PARAMETERS;
    struct port_boo_packet_layout m={0};
    struct port_boo_parameters boo={0};
    if(port_boo_packet_layout(&settings,&m,&boo)!=PORT_BOO_PACKET_OK ||
       m.lrp.first_address<start || m.srp.reserved_end>top ||
       m.nonpaged_start<start ||
       m.srp.resident_end>top ||
       m.irp.resident_end>top || m.lrp.resident_end>top)
        return PORT_SYS_EXEC_BAD_PARAMETERS;
    struct port_ini_packet_area areas[3]={
        {(uint8_t *)(uintptr_t)m.irp.first_address,m.irp.first_address,
         m.irp.resident_end-m.irp.first_address,boo.irp_count,m.irp.packet_bytes,
         m.irp.packet_bytes,(uint32_t)(uintptr_t)&executive_irps.next},
        {(uint8_t *)(uintptr_t)m.lrp.first_address,m.lrp.first_address,
         m.lrp.resident_end-m.lrp.first_address,boo.lrp_count,m.lrp.packet_bytes,
         boo.lrp_minimum>480u?boo.lrp_minimum:480u,
         (uint32_t)(uintptr_t)&executive_lrps.next},
        {(uint8_t *)(uintptr_t)m.srp.first_address,m.srp.first_address,
         m.srp.resident_end-m.srp.first_address,boo.srp_count,m.srp.packet_bytes,
         m.srp.packet_bytes/2u,(uint32_t)(uintptr_t)&executive_srps.next}
    };
    /* Prevalidate all three pools before modifying any resident bytes. */
    for(unsigned i=0;i<3u;i++){
        const struct port_ini_packet_area *a=&areas[i];
        uint64_t used=(uint64_t)a->count*a->packet_bytes;
        uint32_t remain=(a->count && ((a->address+used)&511u))?
                    512u-(uint32_t)((a->address+used)&511u):0u;
        if(a->minimum_bytes>a->packet_bytes ||
           used+remain>a->available_bytes ||
           a->packet_bytes<16u || a->packet_bytes>UINT16_MAX)
            return PORT_SYS_EXEC_BAD_PARAMETERS;
    }
    if(port_ini_packet_init(&areas[0],&executive_irps)!=PORT_INI_PACKETS_OK ||
       port_ini_packet_init(&areas[1],&executive_lrps)!=PORT_INI_PACKETS_OK ||
       port_ini_packet_init(&areas[2],&executive_srps)!=PORT_INI_PACKETS_OK)
        return PORT_SYS_EXEC_BAD_PARAMETERS;
    struct port_vms37_init_params init_params;
    const uint32_t resident_free=m.nonpaged_start-(uint32_t)start;
    if(port_vms37_init_param_defaults(&init_params)!=PORT_VMS37_SYSPARAM_OK||
       resident_free<init_params.nonpaged_dynamic_bytes)
        return PORT_SYS_EXEC_BAD_PARAMETERS;
    /* INIT allocations consume the configured NPAGEDYN region; leave the
     * extra native backing slack outside the original pool limit. */
    executive_free_npages=init_params.nonpaged_dynamic_bytes;
    executive_packet_ready=1u;
    p->result->irp_queue=&executive_irps;
    p->result->lrp_queue=&executive_lrps;
    p->result->srp_queue=&executive_srps;
    p->result->irp_count=executive_irps.packet_count;
    p->result->lrp_count=executive_lrps.packet_count;
    p->result->srp_count=executive_srps.packet_count;
    p->result->signature=PORT_SYS_EXEC_PACKET_RESULT_TAG;
    return PORT_SYS_EXEC_NORMAL;
}


uint32_t port_sys_executive_entry(const struct port_sys_exec_start *p){
    if(!p || p->signature!=PORT_SYS_EXEC_SIGNATURE ||
       p->version!=PORT_SYS_EXEC_VERSION || !p->result || p->reserved ||
       p->pcb_type!=PORT_SYS_EXEC_PCB_KIND)
        return PORT_SYS_EXEC_BAD_PARAMETERS;
    if(p->phase==PORT_SYS_EXEC_INIT_IOQUEUES)
        return port_exe_initialize_lookaside(p);
    if(p->phase==PORT_SYS_EXEC_INIT_LOCKDB){
        /* After INITADP, original INIT.MAR reserves realtime SPTs and
         * initializes the lock-ID free chain, resource hash and process
         * deadlock bitmap. This records the native data structures only;
         * the $ENQ and SPT consumers are not ported yet. */
        if(!executive_packet_ready || executive_lockdb_ready ||
           !p->lock_settings || !executive_free_npages ||
           port_ini_lockdb_init(executive_packet_pages,
               executive_free_npages,p->lock_settings,
               &executive_lockdb)!=PORT_INI_LOCKDB_OK)
            return PORT_SYS_EXEC_BAD_PARAMETERS;
        executive_lockdb_ready=1u;
        p->result->lock_database=&executive_lockdb;
        p->result->lock_max_id=executive_lockdb.max_lock_id;
        p->result->lock_hash_slots=executive_lockdb.hash_slots;
        p->result->lock_process_bitmap_bytes=executive_lockdb.process_bitmap_bytes;
        p->result->signature=PORT_SYS_EXEC_LOCK_RESULT_TAG;
        return PORT_SYS_EXEC_NORMAL;
    }
    if(p->phase==PORT_SYS_EXEC_INIT_PROCVEC){
        /* INIT.MAR INI_PSTATE follows realtime SPT and lock tables.
         * INI_PSTATE's vector must name source-layout PCB records. The
         * scheduler records below are separate diagnostics; both VMS PCB
         * templates still lack PHD/native context activation. */
        if(!executive_lockdb_ready || executive_procvec_ready ||
           !p->proc_settings)return PORT_SYS_EXEC_BAD_PARAMETERS;
        uint32_t used=(executive_lockdb.pool_bytes+3u)&~3u;
        if(used>=executive_free_npages)return PORT_SYS_EXEC_BAD_PARAMETERS;
        struct port_ini_proc_settings settings=*p->proc_settings;
        if((uintptr_t)executive_vms_system_pcb>UINT32_MAX ||
           (uintptr_t)executive_vms_null_pcb>UINT32_MAX ||
           (uintptr_t)executive_vms_swapper_pcb>UINT32_MAX)
            return PORT_SYS_EXEC_BAD_PARAMETERS;
        if(port_v37_pcb_init_syspcb_template(executive_vms_system_pcb,
              sizeof executive_vms_system_pcb,
              (uint32_t)(uintptr_t)executive_vms_system_pcb)!=PORT_V37_PCB_OK)
            return PORT_SYS_EXEC_BAD_PARAMETERS;
        if(port_v37_pcb_set_pix(executive_vms_system_pcb,
              sizeof executive_vms_system_pcb,(uint16_t)settings.max_processes)!=
              PORT_V37_PCB_OK)return PORT_SYS_EXEC_BAD_PARAMETERS;
        const struct port_v37_pcb_seed null_template={
            .address=(uint32_t)(uintptr_t)executive_vms_null_pcb,
            .pid=UINT32_C(0x00010000),.priority=0u,.name="NULL",
            .name_bytes=4u};
        if(port_v37_pcb_init_genpcb(executive_vms_null_pcb,
              sizeof executive_vms_null_pcb,&null_template)!=PORT_V37_PCB_OK)
            return PORT_SYS_EXEC_BAD_PARAMETERS;
        const struct port_v37_pcb_seed swapper_template={
            .address=(uint32_t)(uintptr_t)executive_vms_swapper_pcb,
            .pid=UINT32_C(0x00010001),.priority=16u,.name="SWAPPER",
            .name_bytes=7u};
        if(port_v37_pcb_init_genpcb(executive_vms_swapper_pcb,
              sizeof executive_vms_swapper_pcb,&swapper_template)!=PORT_V37_PCB_OK)
            return PORT_SYS_EXEC_BAD_PARAMETERS;
        settings.system_pcb_address=(uint32_t)(uintptr_t)executive_vms_system_pcb;
        settings.null_pcb_address=(uint32_t)(uintptr_t)executive_vms_null_pcb;
        if(port_ini_procvec_init(executive_packet_pages+used,
                 executive_free_npages-used,&settings,
                 &executive_procvec)!=PORT_INI_PROC_OK)
            return PORT_SYS_EXEC_BAD_PARAMETERS;
        const struct port_v37_sched_ops sched_ops={0,
            executive_vms_address,executive_vms_resolve,0,0};
        if(port_ini_procvec_set_pcb(&executive_procvec,1u,
              (uint32_t)(uintptr_t)executive_vms_swapper_pcb)!=PORT_INI_PROC_OK||
           port_v37_sched_init(&executive_vms_scheduler,&sched_ops)!=
              PORT_V37_SCHED_OK||
           port_v37_sched_chse(&executive_vms_scheduler,
              (uint32_t)(uintptr_t)executive_vms_null_pcb,0u)!=PORT_V37_SCHED_OK||
           port_v37_sched_chse(&executive_vms_scheduler,
              (uint32_t)(uintptr_t)executive_vms_swapper_pcb,0u)!=
              PORT_V37_SCHED_OK||
           port_v37_sched_check(&executive_vms_scheduler)!=PORT_V37_SCHED_OK)
            return PORT_SYS_EXEC_BAD_PARAMETERS;
        executive_null_pcb=(struct port_exe_sched_process){0};
        executive_null_pcb.type=PORT_SYS_EXEC_PCB_KIND;
        executive_null_pcb.state=PORT_EXE_STATE_OTHER;
        executive_system_pcb=(struct port_exe_sched_process){0};
        executive_system_pcb.pid=settings.max_processes;
        executive_system_pcb.type=PORT_SYS_EXEC_PCB_KIND;
        executive_system_pcb.state=PORT_EXE_STATE_OTHER;
        executive_procvec_ready=1u;
        p->result->process_vector=&executive_procvec;
        p->result->system_sched_process=&executive_system_pcb;
        p->result->null_sched_process=&executive_null_pcb;
        p->result->system_vms_pcb=executive_vms_system_pcb;
        p->result->null_vms_pcb=executive_vms_null_pcb;
        p->result->swapper_vms_pcb=executive_vms_swapper_pcb;
        p->result->vms_scheduler=&executive_vms_scheduler;
        p->result->signature=PORT_SYS_EXEC_PV_RESULT_TAG;
        return PORT_SYS_EXEC_NORMAL;
    }
    if(p->phase==PORT_SYS_EXEC_INIT_PHVSWAP){
        /* INIT.MAR INI_PHV and INI_SWAP follow the process PCB vector.
         * Persist both nonpaged DYN records in the actual SYS.EXE's own
         * supervisor allocation, not in the stage-2 bootloader's memory. */
        if(!executive_procvec_ready || executive_phvswap_ready ||
           !p->phv_settings)return PORT_SYS_EXEC_BAD_PARAMETERS;
        if(p->phv_settings->system_pix!=executive_procvec.system_pix)
            return PORT_SYS_EXEC_BAD_PARAMETERS;
        uint32_t lockused=(executive_lockdb.pool_bytes+3u)&~3u;
        uint32_t pvused=(executive_procvec.allocated_bytes+3u)&~3u;
        if(lockused>executive_free_npages ||
           pvused>executive_free_npages-lockused)
            return PORT_SYS_EXEC_BAD_PARAMETERS;
        uint32_t used=lockused+pvused;
        if(used>=executive_free_npages ||
           port_ini_phvswap_init(executive_packet_pages+used,
                 executive_free_npages-used,p->phv_settings,
                 &executive_phvswap)!=PORT_INI_PHVSWAP_OK)
            return PORT_SYS_EXEC_BAD_PARAMETERS;
        if(!executive_phvswap.phv_pix_base ||
           !executive_phvswap.swapper_p0br ||
           port_ini_phvswap_pix(&executive_phvswap,
               executive_phvswap.phv_slots-1u)!=executive_procvec.system_pix ||
           port_ini_phvswap_refcount(&executive_phvswap,
               executive_phvswap.phv_slots-1u)!=1024)
            return PORT_SYS_EXEC_BAD_PARAMETERS;
        executive_phvswap_ready=1u;
        p->result->process_header_vector=&executive_phvswap;
        p->result->signature=PORT_SYS_EXEC_PHV_RESULT_TAG;
        return PORT_SYS_EXEC_NORMAL;
    }
    if(p->phase==PORT_SYS_EXEC_INIT_MPW){
        /* Original INIT.MAR INI_MPW follows INI_SWAP: the modified-page
         * writer PTEs and PHV indexes share one nonpaged DYN record. */
        if(!executive_phvswap_ready || executive_mpw_ready || !p->mpw_settings)
            return PORT_SYS_EXEC_BAD_PARAMETERS;
        uint32_t lock=(executive_lockdb.pool_bytes+3u)&~3u;
        uint32_t proc=(executive_procvec.allocated_bytes+3u)&~3u;
        uint32_t phv=(executive_phvswap.pool_bytes+3u)&~3u;
        if(lock>executive_free_npages ||
           proc>executive_free_npages-lock ||
           phv>executive_free_npages-lock-proc)
            return PORT_SYS_EXEC_BAD_PARAMETERS;
        uint32_t used=lock+proc+phv;
        if(used>=executive_free_npages ||
           port_ini_mpw_init(executive_packet_pages+used,
               executive_free_npages-used,p->mpw_settings,
               &executive_mpw)!=PORT_INI_MPW_OK)
            return PORT_SYS_EXEC_BAD_PARAMETERS;
        executive_mpw_ready=1u;
        p->result->modified_page_writer_map=&executive_mpw;
        p->result->signature=PORT_SYS_EXEC_MPW_RESULT_TAG;
        return PORT_SYS_EXEC_NORMAL;
    }
    if(p->phase==PORT_SYS_EXEC_INIT_PFLVEC){
        /* INI_PFLVEC, INIT.MAR 1142-1165: nonpaged PTR header and
         * pointer slots share the resident SYS.EXE allocation. Only
         * the original MMG$GL_NULLPFL address belongs in each slot.
         * The shell's WCB/VBN and a real PFL service do not exist yet. */
        if(!executive_mpw_ready || executive_pflvec_ready ||
           executive_interrupt_stack_ready || !p->pfl_settings)
            return PORT_SYS_EXEC_BAD_PARAMETERS;
        const struct port_ini_pflvec_settings *s=p->pfl_settings;
        if(s->null_pfl_address || s->shell_base_address ||
           s->shell_system_window)
            return PORT_SYS_EXEC_BAD_PARAMETERS;
        struct port_ini_pflvec_settings settings=*s;
        settings.null_pfl_address=(uint32_t)(uintptr_t)executive_nullpfl_uninitialized;
        uint32_t lock=(executive_lockdb.pool_bytes+3u)&~3u;
        uint32_t proc=(executive_procvec.allocated_bytes+3u)&~3u;
        uint32_t phv=(executive_phvswap.pool_bytes+3u)&~3u;
        uint32_t mpw=(executive_mpw.allocated_bytes+3u)&~3u;
        if(lock>executive_free_npages ||
           proc>executive_free_npages-lock ||
           phv>executive_free_npages-lock-proc ||
           mpw>executive_free_npages-lock-proc-phv)
            return PORT_SYS_EXEC_BAD_PARAMETERS;
        uint32_t used=lock+proc+phv+mpw;
        if(used>=executive_free_npages ||
           port_ini_pflvec_init(executive_packet_pages+used,
                    executive_free_npages-used,&settings,
                    &executive_pflvec)!=PORT_INI_PFL_OK)
            return PORT_SYS_EXEC_BAD_PARAMETERS;
        executive_pflvec_ready=1u;
        p->result->pagefile_vector=&executive_pflvec;
        p->result->signature=PORT_SYS_EXEC_PFL_RESULT_TAG;
        return PORT_SYS_EXEC_NORMAL;
    }
    if(p->phase==PORT_SYS_EXEC_INIT_INTSTKLM){
        /* Original INIT.MAR INI_INTSTKLM computes EXE$GL_INTSTKLM and
         * LCK$GB_MAXDEPTH from the live interrupt-stack top, number of
         * VAX 512-byte pages and LCK$GL_EXTRASTK. On IA-32 this request
         * describes the actual 386 TSS privilege-transition stack. It is
         * not a claim of VAX IPL hardware or an active lock manager. */
        if(!executive_pflvec_ready || executive_interrupt_stack_ready ||
           !p->intstack_settings)
            return PORT_SYS_EXEC_BAD_PARAMETERS;
        const struct port_sys_exec_intstack_settings *s=p->intstack_settings;
        if(port_ini_interrupt_stack_limits(s->stack_high,s->stack_pages,
               s->lock_extra_bytes,&executive_interrupt_stack)!=PORT_INI_PFL_OK ||
           !executive_interrupt_stack.low ||
           executive_interrupt_stack.high!=s->stack_high)
            return PORT_SYS_EXEC_BAD_PARAMETERS;
        executive_interrupt_stack_ready=1u;
        p->result->interrupt_stack=&executive_interrupt_stack;
        p->result->lock_max_resource_depth=
               executive_interrupt_stack.max_resource_depth;
        p->result->signature=PORT_SYS_EXEC_INTSTK_RESULT_TAG;
        return PORT_SYS_EXEC_NORMAL;
    }
    if(p->phase==PORT_SYS_EXEC_INIT_SYSWCB){
        /* Source INIT.MAR 1194-1212.  Real on-disk normalized map
         * produced by the guest's SYSBOOT, consumed by resident SYS.EXE.
         * WCB's remaining type/flags/fields need their real DEC layout;
         * do not advertise an ACP-capable window or paged shell. */
        if(!executive_interrupt_stack_ready || !executive_pflvec_ready ||
           executive_system_wcb_ready || !p->syswcb_settings)
            return PORT_SYS_EXEC_BAD_PARAMETERS;
        const struct port_sys_exec_wcb_settings *s=p->syswcb_settings;
        if(!s->normalized_boot_map || !s->boot_map_capacity ||
           !s->expected_extent_count || !s->expected_total_blocks ||
           !s->expected_first_lbn || (uintptr_t)s->normalized_boot_map&3u)
            return PORT_SYS_EXEC_BAD_PARAMETERS;
        /* Do not permit the caller to supply an invented count. */
        uint32_t bytes=(uint32_t)s->normalized_boot_map[0] |
           ((uint32_t)s->normalized_boot_map[1]<<8) |
           ((uint32_t)s->normalized_boot_map[2]<<16) |
           ((uint32_t)s->normalized_boot_map[3]<<24);
        if(bytes>UINT32_MAX-4u || (bytes&7u) ||
           bytes/8u!=s->expected_extent_count ||
           (uint64_t)bytes+4u>s->boot_map_capacity)
            return PORT_SYS_EXEC_BAD_PARAMETERS;
        uint32_t blocks=0u,first_lbn=0u;
        for(uint32_t i=0u;i<s->expected_extent_count;i++){
            const uint8_t *slot=s->normalized_boot_map+4u+8u*i;
            uint32_t n=(uint32_t)slot[0]|((uint32_t)slot[1]<<8)|
                 ((uint32_t)slot[2]<<16)|((uint32_t)slot[3]<<24);
            uint32_t l=(uint32_t)slot[4]|((uint32_t)slot[5]<<8)|
                 ((uint32_t)slot[6]<<16)|((uint32_t)slot[7]<<24);
            if(!n || n>65535u || l>UINT32_MAX-n ||
               blocks>UINT32_MAX-n)
                return PORT_SYS_EXEC_BAD_PARAMETERS;
            if(!i)first_lbn=l;
            blocks+=n;
        }
        if(first_lbn!=s->expected_first_lbn ||
           blocks!=s->expected_total_blocks)
            return PORT_SYS_EXEC_BAD_PARAMETERS;
        /* The guest's own linked SYS.EXE header is deliberately limited
         * to tested structural fields. This is NOT the full VAX WCB. */
        for(uint32_t i=0u;i<sizeof executive_wcb_header;i++)
            executive_wcb_header[i]=0u;
        executive_wcb_header[4]=sizeof executive_wcb_header;
        const struct port_ini_syswcb_layout layout={
            sizeof executive_wcb_header,4u,6u};
        if(port_ini_syswcb_from_boot_map(s->normalized_boot_map,
                 s->boot_map_capacity,executive_wcb_header,
                 sizeof executive_wcb_header,&layout,
                 &executive_system_wcb)!=PORT_INI_SYSWCB_OK ||
           executive_system_wcb.mapping_count!=s->expected_extent_count ||
           executive_system_wcb.record_length!=
                sizeof executive_wcb_header+6u*s->expected_extent_count)
            return PORT_SYS_EXEC_BAD_PARAMETERS;
        uint32_t check=0u;
        for(uint32_t i=0u;i<executive_system_wcb.mapping_count;i++){
            uint16_t n=0u;uint32_t l=0u;
            if(port_ini_syswcb_retrieval(&executive_system_wcb,i,
                  &n,&l)!=PORT_INI_SYSWCB_OK || !n ||
               (i==0u && l!=first_lbn) || check>UINT32_MAX-n)
                return PORT_SYS_EXEC_BAD_PARAMETERS;
            check+=n;
        }
        if(check!=blocks)return PORT_SYS_EXEC_BAD_PARAMETERS;
        executive_system_wcb_ready=1u;
        p->result->system_wcb=&executive_system_wcb;
        p->result->signature=PORT_SYS_EXEC_SYSWCB_RESULT_TAG;
        return PORT_SYS_EXEC_NORMAL;
    }
    if(p->phase==PORT_SYS_EXEC_INIT_DPT_STAGES){
        /* INIT.MAR 1232-1239 requires the 3 actual linked DPT driver
         * images. Stage source-defined headers, but DO NOT put them on
         * IOC$GL_DPTLIST: their DPT$W_SIZE=0 and DDTs are incomplete. */
        if(!executive_system_wcb_ready || executive_dpt_stages_ready ||
           executive_null_fdt_ready ||
           !port_v37_dpt_stage_mb(executive_dpt_stages[0]) ||
           !port_v37_dpt_stage_nl(executive_dpt_stages[1]) ||
           !port_v37_dpt_stage_op(executive_dpt_stages[2]) ||
           !port_v37_dpt_validate_staged((const uint8_t (*)[PORT_V37_DPT_STAGE_BYTES])executive_dpt_stages))
            return PORT_SYS_EXEC_BAD_PARAMETERS;
        executive_dpt_stages_ready=1u;
        p->result->dpt_stages=(const uint8_t (*)[PORT_V37_DPT_STAGE_BYTES])executive_dpt_stages;
        p->result->signature=PORT_SYS_EXEC_DPT_RESULT_TAG;
        return PORT_SYS_EXEC_NORMAL;
    }
    if(p->phase==PORT_SYS_EXEC_INIT_NL_FDT){
        /* Original NLDRIVER.MAR 49-133: the native SYS.EXE already
         * contains the exact five read/write/EOF FDT decisions. Expose
         * its real text address without inventing IOC or FINISHIOC. */
        if(!executive_system_wcb_ready || !executive_dpt_stages_ready ||
           executive_null_fdt_ready ||
           executive_syslog_ready || port_nl_driver_dpt.ucb_size!=116u ||
           port_nl_driver_dpt.count!=5u ||
           port_nl_driver_dpt.device_functions[0]!=PORT_SYS85_READLBLK ||
           port_nl_driver_dpt.device_functions[1]!=PORT_SYS85_READVBLK ||
           port_nl_driver_dpt.device_functions[2]!=PORT_SYS85_WRITEOF ||
           port_nl_driver_dpt.device_functions[3]!=PORT_SYS85_WRITELBLK ||
           port_nl_driver_dpt.device_functions[4]!=PORT_SYS85_WRITEVBLK)
            return PORT_SYS_EXEC_BAD_PARAMETERS;
        executive_null_fdt_ready=1u;
        p->result->null_driver_dpt=&port_nl_driver_dpt;
        p->result->null_driver_io=port_nl_driver_io;
        p->result->signature=PORT_SYS_EXEC_NL_RESULT_TAG;
        return PORT_SYS_EXEC_NORMAL;
    }
    if(p->phase==PORT_SYS_EXEC_INIT_SYSLOG){
        /* INI_LOG consumes initialized nonpaged pool. The verified VMS
         * DYN$C_LOG=11 is used; IOC/DDB and SWAPINIT remain absent,
         * so these two records cannot yet be active logical names. */
        if(!executive_system_wcb_ready || !executive_pflvec_ready ||
           !executive_null_fdt_ready || executive_syslog_ready)
            return PORT_SYS_EXEC_BAD_PARAMETERS;
        const uint32_t size=executive_free_npages;
        uint32_t used=0u;
        const uint32_t chunks[5]={
            executive_lockdb.pool_bytes,
            executive_procvec.allocated_bytes,
            executive_phvswap.pool_bytes,
            executive_mpw.allocated_bytes,
            executive_pflvec.allocated_bytes};
        for(unsigned i=0;i<5u;i++){
            if(chunks[i]>UINT32_MAX-3u)return PORT_SYS_EXEC_BAD_PARAMETERS;
            uint32_t part=(chunks[i]+3u)&~3u;
            if(part>size-used)return PORT_SYS_EXEC_BAD_PARAMETERS;
            used+=part;
        }
        if(used>UINT32_MAX-15u)return PORT_SYS_EXEC_BAD_PARAMETERS;
        used=(used+15u)&~15u;
        if(used>size || size-used<2u*PORT_INI_LOG_TEMPLATE_BYTES)
            return PORT_SYS_EXEC_BAD_PARAMETERS;
        port_ini_log_templates(executive_disklog,executive_sysdevlog);
        struct port_ini_syslog_settings settings={
            .size_field_offset=PORT_INI_LOG_SIZE_OFFSET,
            .pool_address=(uint32_t)(uintptr_t)(executive_packet_pages+used),
            .allocation_alignment=16u};
        if(port_ini_syslog_init(executive_packet_pages+used,size-used,
               executive_disklog,sizeof executive_disklog,
               executive_sysdevlog,sizeof executive_sysdevlog,&settings,
               &executive_syslog)!=PORT_INI_SYSLOG_OK)
            return PORT_SYS_EXEC_BAD_PARAMETERS;
        uint64_t pool_offset=(uint64_t)used+
                             executive_syslog.consumed_bytes;
        pool_offset=(pool_offset+7u)&~UINT64_C(7);
        if(size>sizeof executive_packet_pages||pool_offset>size||
           size-pool_offset<sizeof(struct port_exe_np_free))
            return PORT_SYS_EXEC_BAD_PARAMETERS;
        const uint32_t pool_bytes=((size-(uint32_t)pool_offset)&~7u);
        if(port_exe_np_pool_init(&executive_np_pool,
               executive_packet_pages+(uint32_t)pool_offset,pool_bytes,
               &executive_qio_pool_sync)!=PORT_EXE_NP_POOL_OK)
            return PORT_SYS_EXEC_BAD_PARAMETERS;
        executive_np_pool_ready=1u;
        executive_syslog_ready=1u;
        p->result->syslog=&executive_syslog;
        p->result->logical_table[0]=executive_syslog.logical_table[0];
        p->result->logical_table[1]=executive_syslog.logical_table[1];
        p->result->signature=PORT_SYS_EXEC_SYSLOG_RESULT_TAG;
        return PORT_SYS_EXEC_NORMAL;
    }
    if(p->phase==PORT_SYS_EXEC_INIT_DQ_FDT){
        /* DRIVER/SRC/DQDRIVER.MAR 496-590, invoked only after real
         * INILOA selected native ATA hardware. No DQDDT/ACP is linked:
         * DQ FDT discovery MUST return UNBOUND for every legal function. */
        if(!executive_bootdev_scan_ready || executive_dq_fdt_ready ||
           executive_bootdev_scan.boot_kind!=PORT_INI_PC_ATA_PIO ||
           port_dq_fdt_build(&executive_dq_fdt,sizeof executive_dq_fdt)!=PORT_DQ_FDT_OK ||
           port_dq_fdt_validate(&executive_dq_fdt,sizeof executive_dq_fdt)!=PORT_DQ_FDT_OK ||
           port_dq_fdt_dispatch(&executive_dq_fdt,sizeof executive_dq_fdt,
                                PORT_DQ_READLBLK)!=PORT_DQ_FDT_UNBOUND ||
           port_dq_fdt_dispatch(&executive_dq_fdt,sizeof executive_dq_fdt,
                                PORT_DQ_WRITELBLK)!=PORT_DQ_FDT_UNBOUND ||
           port_dq_fdt_dispatch(&executive_dq_fdt,sizeof executive_dq_fdt,
                                63u)!=PORT_DQ_FDT_ILLEGAL)
            return PORT_SYS_EXEC_BAD_PARAMETERS;
        executive_dq_fdt_ready=1u;
        p->result->dq_source_fdt=&executive_dq_fdt;
        p->result->dq_fdt_classify=port_dq_fdt_classify;
        p->result->signature=PORT_SYS_EXEC_DQ_RESULT_TAG;
        return PORT_SYS_EXEC_NORMAL;
    }
    if(p->phase==PORT_SYS_EXEC_INIT_DQ_BOOTREAD){
        /* Source DQDRIVER.MAR PREPROCESS/FDISPATCH/FUNCXT 640-940:
         * a physical read bypasses unavailable software volume-valid
         * and hands one verified LBN to the real PC ATA 512-byte PIO.
         * No disk DDT, ACP, normal $QIO or AST may be invented. */
        if(!executive_dq_fdt_ready||!executive_devicedb_ready||
           !executive_bootdev_scan_ready||executive_dq_bootread_ready||
           executive_bootdev_scan.boot_kind!=PORT_INI_PC_ATA_PIO||
           executive_devicedb_result.incomplete_driver_mask!=7u||
           !p->dq_boot_io || !p->dq_boot_io->read_sector ||
           p->dq_boot_io->boot_unit!=0u || p->dq_boot_io->reserved ||
           (uintptr_t)executive_dq_boot_irp.bytes>UINT32_MAX)
            return PORT_SYS_EXEC_BAD_PARAMETERS;
        const uint32_t irp=(uint32_t)(uintptr_t)executive_dq_boot_irp.bytes;
        if(port_v37_irp_prepare(executive_dq_boot_irp.bytes,
                PORT_V37_IRP_SIZE,irp,executive_devicedb_result.ucb[0],
                PORT_DQ_READPBLK)!=PORT_V37_QIO_OK)
            return PORT_SYS_EXEC_BAD_PARAMETERS;
        port_v37_write16(executive_dq_boot_irp.bytes+42u,PORT_DQ_PHYSIO);
        port_v37_write16(executive_dq_boot_irp.bytes+50u,PORT_DQ_BOOTREAD_BYTES);
        port_v37_write32(executive_dq_boot_irp.bytes+PORT_V37_IRP_MEDIA,
                         PORT_DQ_BOOTREAD_LBN);
        const struct port_dq_bootread_request dq={
            .fdt=&executive_dq_fdt,
            .device_db=executive_devicedb_bytes.data,
            .device_info=&executive_devicedb_result,
            .irp=executive_dq_boot_irp.bytes,.irp_bytes=PORT_V37_IRP_SIZE,
            .sector=executive_dq_boot_sector.words,
            .sector_bytes=PORT_DQ_BOOTREAD_BYTES,
            .boot_unit=p->dq_boot_io->boot_unit,
            .read_sector=p->dq_boot_io->read_sector,
            .read_context=p->dq_boot_io->context};
        if(port_dq_bootread_execute(&dq,&executive_dq_bootread_result)!=
                         PORT_DQ_BOOTREAD_OK ||
           executive_dq_bootread_result.lbn!=1u ||
           executive_dq_bootread_result.bytes!=512u ||
           executive_dq_bootread_result.irp_status!=1u ||
           executive_dq_bootread_result.irp_second!=512u ||
           executive_dq_boot_sector.words[0]!=1u)
            return PORT_SYS_EXEC_BAD_PARAMETERS;
        /* Independent physical LBN0 read after the original Files-11
         * home block; preserve both IRP results in separate resident RAM. */
        if(port_v37_irp_prepare(executive_dq_bootzero_irp.bytes,
               PORT_V37_IRP_SIZE,(uint32_t)(uintptr_t)executive_dq_bootzero_irp.bytes,
               executive_devicedb_result.ucb[0],PORT_DQ_READPBLK)!=PORT_V37_QIO_OK)
            return PORT_SYS_EXEC_BAD_PARAMETERS;
        port_v37_write16(executive_dq_bootzero_irp.bytes+42u,PORT_DQ_PHYSIO);
        port_v37_write16(executive_dq_bootzero_irp.bytes+50u,PORT_DQ_BOOTREAD_BYTES);
        port_v37_write32(executive_dq_bootzero_irp.bytes+PORT_V37_IRP_MEDIA,0u);
        struct port_dq_bootread_request physical=dq;
        physical.irp=executive_dq_bootzero_irp.bytes;
        physical.sector=executive_dq_bootzero_sector.words;
        if(port_dq_bootread_execute_physical(&physical,&executive_dq_bootzero_result,0u)
             !=PORT_DQ_BOOTREAD_OK || executive_dq_bootzero_result.lbn!=0u ||
           executive_dq_bootzero_result.bytes!=512u ||
           executive_dq_bootzero_result.irp_status!=PORT_SYS85_NORMAL ||
           executive_dq_bootzero_result.ucb_io_count!=2u ||
           executive_dq_bootzero_result.sector_checksum!=
                port_dq_bootread_checksum(executive_dq_bootzero_sector.words))
            return PORT_SYS_EXEC_BAD_PARAMETERS;
        /* FIL$MOUNT locates INDEXF.SYS first header at home IBMAPLBN +
         * IBMAPSIZE. Read real ATA data using a third reserved IRP. */
        uint32_t index_lbn=0u;
        if(port_dq_index_location((const uint8_t *)executive_dq_boot_sector.words,
                &index_lbn)!=PORT_DQ_INDEX_OK ||
           port_v37_irp_prepare(executive_dq_index_irp.bytes,
               PORT_V37_IRP_SIZE,(uint32_t)(uintptr_t)executive_dq_index_irp.bytes,
               executive_devicedb_result.ucb[0],PORT_DQ_READPBLK)!=PORT_V37_QIO_OK)
            return PORT_SYS_EXEC_BAD_PARAMETERS;
        port_v37_write16(executive_dq_index_irp.bytes+42u,PORT_DQ_PHYSIO);
        port_v37_write16(executive_dq_index_irp.bytes+50u,PORT_DQ_BOOTREAD_BYTES);
        port_v37_write32(executive_dq_index_irp.bytes+PORT_V37_IRP_MEDIA,index_lbn);
        struct port_dq_bootread_request indexed=dq;
        indexed.irp=executive_dq_index_irp.bytes;
        indexed.sector=executive_dq_index_sector.words;
        if(port_dq_index_read(&indexed,executive_dq_boot_sector.words,
               &executive_dq_index_io,&executive_dq_index_result)!=PORT_DQ_INDEX_OK ||
           executive_dq_index_result.lbn!=index_lbn ||
           executive_dq_index_result.file_number!=1u ||
           executive_dq_index_result.file_sequence!=1u ||
           executive_dq_index_result.io_count!=3u)
            return PORT_SYS_EXEC_BAD_PARAMETERS;
        /* FIL$RDCHKFILHDR: MFD FID 4,4,0, resolved through the real
         * INDEXF.SYS FH2 retrieval-pointer map. This is a fourth IRP. */
        uint32_t mfd_vbn=0u,mfd_lbn=0u;
        if(port_dq_mfd_location((const uint8_t *)executive_dq_boot_sector.words,
                  (const uint8_t *)executive_dq_index_sector.words,
                  &mfd_vbn,&mfd_lbn)!=PORT_DQ_MFD_OK||
           port_v37_irp_prepare(executive_dq_mfd_irp.bytes,
                  PORT_V37_IRP_SIZE,(uint32_t)(uintptr_t)executive_dq_mfd_irp.bytes,
                  executive_devicedb_result.ucb[0],PORT_DQ_READPBLK)!=PORT_V37_QIO_OK)
            return PORT_SYS_EXEC_BAD_PARAMETERS;
        port_v37_write16(executive_dq_mfd_irp.bytes+42u,PORT_DQ_PHYSIO);
        port_v37_write16(executive_dq_mfd_irp.bytes+50u,PORT_DQ_BOOTREAD_BYTES);
        port_v37_write32(executive_dq_mfd_irp.bytes+PORT_V37_IRP_MEDIA,mfd_lbn);
        struct port_dq_bootread_request mfdq=dq;
        mfdq.irp=executive_dq_mfd_irp.bytes;
        mfdq.sector=executive_dq_mfd_sector.words;
        if(port_dq_mfd_read(&mfdq,executive_dq_boot_sector.words,
               executive_dq_index_sector.words,&executive_dq_mfd_io,
               &executive_dq_mfd_result)!=PORT_DQ_MFD_OK ||
           executive_dq_mfd_result.vbn!=mfd_vbn||
           executive_dq_mfd_result.lbn!=mfd_lbn||
           executive_dq_mfd_result.fid!=4u||
           executive_dq_mfd_result.sequence!=4u||
           executive_dq_mfd_result.io_count!=4u)
            return PORT_SYS_EXEC_BAD_PARAMETERS;
        /* FIL$READVBN then FIL$FINDFILID in source order. The first
         * directory record carries SYSEXE.DIR and SYS.EXE FIDs; find
         * them from live Files-11 disk bytes, not hardcoded name claims. */
        uint32_t mfd_data_lbn=0u;
        if(port_dq_dir_map_vbn((const uint8_t *)executive_dq_mfd_sector.words,
                1u,&mfd_data_lbn)!=PORT_DQ_DIR_OK)
            return PORT_SYS_EXEC_BAD_PARAMETERS;
        const uint32_t native_lbn[3]={mfd_data_lbn,0u,0u};
        (void)native_lbn;
        const uint32_t start_lbn=mfd_data_lbn;
        for(unsigned step=0;step<3u;step++){
            uint8_t *irp=executive_dq_dir_irps.bytes+step*PORT_V37_IRP_SIZE;
            uint16_t *sector=step==0u?executive_dq_dir_mfd_data.words:
                             step==1u?executive_dq_dir_sysexe_header.words:
                                      executive_dq_dir_sysexe_data.words;
            uint32_t lbn=start_lbn;
            if(step==1u){
                /* INDEXF.SYS FH2 maps the file-header VBN for the
                 * directory FID returned by MFD directory scanning. */
                const uint64_t index_vbn=(uint64_t)port_dq_index_word(
                       (const uint8_t *)executive_dq_boot_sector.words,32u)+
                       4u*(uint64_t)port_dq_index_word(
                       (const uint8_t *)executive_dq_boot_sector.words,14u)+
                       executive_sysexe_dir_fid.number;
                if(index_vbn>UINT32_MAX||
                   port_dq_dir_map_vbn((const uint8_t *)executive_dq_index_sector.words,
                       (uint32_t)index_vbn,&lbn)!=PORT_DQ_DIR_OK)
                    return PORT_SYS_EXEC_BAD_PARAMETERS;
            }else if(step==2u){
                if(port_dq_dir_map_vbn((const uint8_t *)executive_dq_dir_sysexe_header.words,
                    1u,&lbn)!=PORT_DQ_DIR_OK)
                    return PORT_SYS_EXEC_BAD_PARAMETERS;
            }
            if(port_v37_irp_prepare(irp,PORT_V37_IRP_SIZE,
                   (uint32_t)(uintptr_t)irp,
                   executive_devicedb_result.ucb[0],PORT_DQ_READPBLK)!=PORT_V37_QIO_OK)
                return PORT_SYS_EXEC_BAD_PARAMETERS;
            port_v37_write16(irp+42u,PORT_DQ_PHYSIO);
            port_v37_write16(irp+50u,PORT_DQ_BOOTREAD_BYTES);
            port_v37_write32(irp+PORT_V37_IRP_MEDIA,lbn);
            struct port_dq_bootread_request directory=dq;
            directory.irp=irp;directory.sector=sector;
            if(port_dq_bootread_execute_physical(&directory,&executive_dq_dir_io[step],lbn)
                 !=PORT_DQ_BOOTREAD_OK||
               executive_dq_dir_io[step].ucb_io_count!=5u+step)
                return PORT_SYS_EXEC_BAD_PARAMETERS;
            if(step==0u){
                if(port_dq_dir_find((const uint8_t *)sector,
                      "SYSEXE.DIR",10u,1u,&executive_sysexe_dir_fid)!=PORT_DQ_DIR_OK)
                    return PORT_SYS_EXEC_BAD_PARAMETERS;
            }else if(step==1u){
                if(port_dq_dir_validate_header((const uint8_t *)sector,
                    &executive_sysexe_dir_fid)!=PORT_DQ_DIR_OK)
                    return PORT_SYS_EXEC_BAD_PARAMETERS;
            }else{
                if(port_dq_dir_find((const uint8_t *)sector,
                      "SYS.EXE",7u,1u,&executive_sysexecutable_fid)!=PORT_DQ_DIR_OK)
                    return PORT_SYS_EXEC_BAD_PARAMETERS;
            }
        }
        /* Independently open SYS.EXE FID found in SYSEXE.DIR. This
         * verifies the original disk hierarchy all the way to FH2 and
         * a real first image sector, without executing image code. */
        uint32_t image_header_lbn=0u;
        if(port_dq_image_header_lbn((const uint8_t *)executive_dq_boot_sector.words,
             (const uint8_t *)executive_dq_index_sector.words,
             &executive_sysexecutable_fid,&image_header_lbn)!=PORT_DQ_IMAGE_OK)
            return PORT_SYS_EXEC_BAD_PARAMETERS;
        uint32_t image_data_lbn=0u,elf_entry=0u;
        uint16_t elf_segments=0u;
        for(unsigned step=0u;step<2u;step++){
            uint8_t *irp=executive_dq_image_irps.bytes+step*PORT_V37_IRP_SIZE;
            uint16_t *sector=step==0u?executive_dq_image_header.words:
                                        executive_dq_image_data.words;
            uint32_t lbn=step==0u?image_header_lbn:image_data_lbn;
            if(port_v37_irp_prepare(irp,PORT_V37_IRP_SIZE,
                   (uint32_t)(uintptr_t)irp,
                   executive_devicedb_result.ucb[0],PORT_DQ_READPBLK)!=PORT_V37_QIO_OK)
                return PORT_SYS_EXEC_BAD_PARAMETERS;
            port_v37_write16(irp+42u,PORT_DQ_PHYSIO);
            port_v37_write16(irp+50u,PORT_DQ_BOOTREAD_BYTES);
            port_v37_write32(irp+PORT_V37_IRP_MEDIA,lbn);
            struct port_dq_bootread_request qimg=dq;
            qimg.irp=irp;qimg.sector=sector;
            if(port_dq_bootread_execute_physical(&qimg,&executive_dq_image_io[step],lbn)
                 !=PORT_DQ_BOOTREAD_OK||
               executive_dq_image_io[step].ucb_io_count!=8u+step)
                return PORT_SYS_EXEC_BAD_PARAMETERS;
            if(step==0u){
                if(port_dq_image_verify_fh2((const uint8_t *)sector,
                    &executive_sysexecutable_fid)!=PORT_DQ_IMAGE_OK ||
                   port_dq_image_first_data((const uint8_t *)sector,
                    &image_data_lbn)!=PORT_DQ_IMAGE_OK)
                    return PORT_SYS_EXEC_BAD_PARAMETERS;
            }else{
                if(port_dq_image_check_elf32((const uint8_t *)sector,
                        &elf_entry,&elf_segments)!=PORT_DQ_IMAGE_OK)
                    return PORT_SYS_EXEC_BAD_PARAMETERS;
            }
        }
        uint32_t text_lbn=0u,text_va=0u,text_offset=0u;
        if(port_dq_image_text_lbn((const uint8_t *)executive_dq_image_header.words,
             (const uint8_t *)executive_dq_image_data.words,
             &text_lbn,&text_va,&text_offset)!=PORT_DQ_IMAGE_OK||
           text_va<PORT_SYS_ELF_START||text_va>PORT_SYS_ELF_LIMIT-512u)
            return PORT_SYS_EXEC_BAD_PARAMETERS;
        /* Original FIL$READVBN maps every PT_LOAD text VBN through
         * INDEXF.SYS FH2. The final sector may contain fewer than 512
         * executable bytes; never compare unrelated trailing disk data. */
        uint32_t text_elf_filesz=0u;
        if(port_dq_image_text_filesz((const uint8_t *)executive_dq_image_data.words,
               text_va,text_offset,&text_elf_filesz)!=PORT_DQ_IMAGE_OK)
            return PORT_SYS_EXEC_BAD_PARAMETERS;
        const uint32_t text_sectors=(text_elf_filesz+511u)/512u;
        if(text_sectors<16u||text_sectors>512u)
            return PORT_SYS_EXEC_BAD_PARAMETERS;
        for(uint32_t n=0u;n<text_sectors;n++){
            uint32_t sector_lbn=0u,sector_va=0u,valid_bytes=0u;
            if(port_dq_image_text_sector((const uint8_t *)executive_dq_image_header.words,
                 (const uint8_t *)executive_dq_image_data.words,
                 n,&sector_lbn,&sector_va,&valid_bytes)!=PORT_DQ_IMAGE_OK||
               !valid_bytes||valid_bytes>512u||
               (n==0u&&(sector_lbn!=text_lbn||sector_va!=text_va)))
                return PORT_SYS_EXEC_BAD_PARAMETERS;
            uint8_t *text_irp=executive_dq_text_irp.bytes;
            if(port_v37_irp_prepare(text_irp,PORT_V37_IRP_SIZE,
                  (uint32_t)(uintptr_t)text_irp,
                  executive_devicedb_result.ucb[0],PORT_DQ_READPBLK)!=PORT_V37_QIO_OK)
                return PORT_SYS_EXEC_BAD_PARAMETERS;
            port_v37_write16(text_irp+42u,PORT_DQ_PHYSIO);
            port_v37_write16(text_irp+50u,PORT_DQ_BOOTREAD_BYTES);
            port_v37_write32(text_irp+PORT_V37_IRP_MEDIA,sector_lbn);
            struct port_dq_bootread_request text_req=dq;
            text_req.irp=text_irp;text_req.sector=executive_dq_text_sector.words;
            if(port_dq_bootread_execute_physical(&text_req,&executive_dq_text_io,
                 sector_lbn)!=PORT_DQ_BOOTREAD_OK||
               executive_dq_text_io.ucb_io_count!=10u+n)
                return PORT_SYS_EXEC_BAD_PARAMETERS;
            const uint8_t *resident=(const uint8_t *)(uintptr_t)sector_va;
            const uint8_t *disk=(const uint8_t *)executive_dq_text_sector.words;
            for(unsigned byte=0u;byte<valid_bytes;byte++){
                if(resident[byte]!=disk[byte])return PORT_SYS_EXEC_BAD_PARAMETERS;
            }
        }
        /* Read both separately loaded native supervisor text images through
         * original FIL$FINDFILID -> FH2 -> FIL$READVBN on this DQ adapter.
         * The actual resident bytes must equal disk contents, even across
         * the last partial ELF page. No driver publication occurs. */
        static const struct {
            char name[16];uint8_t length;
            uint32_t start,end;
        } aux_files[2]={{"SYSLOA386.EXE",13u,0x003e0000u,0x003f0000u},
                         {"INILOA.EXE",10u,0x00380000u,0x003e0000u}};
        uint32_t completed=9u+text_sectors;
        for(unsigned file=0u;file<2u;file++){
            struct port_dq_dir_fid fid={0};
            uint32_t header_lbn=0u,data_lbn=0u;
            uint32_t matched_vbn=0u;
            struct executive_f11_context dir_io={dq,completed,0u,
                executive_devicedb_result.ucb[0]};
            if(port_dq_dir_find_blocks(
                 (const uint8_t *)executive_dq_dir_sysexe_header.words,
                 aux_files[file].name,aux_files[file].length,1u,1u,
                 executive_f11_native_sector,&dir_io,&fid,&matched_vbn)!=PORT_DQ_DIR_OK||
               matched_vbn!=1u||dir_io.reads!=1u||
               port_dq_image_header_lbn(
                  (const uint8_t *)executive_dq_boot_sector.words,
                  (const uint8_t *)executive_dq_index_sector.words,
                  &fid,&header_lbn)!=PORT_DQ_IMAGE_OK)
                return PORT_SYS_EXEC_BAD_PARAMETERS;
            completed+=dir_io.reads;
            for(unsigned step=0u;step<2u;step++){
                uint32_t lbn=step==0u?header_lbn:data_lbn;
                uint8_t *irp=executive_dq_aux_irp.bytes;
                if(port_v37_irp_prepare(irp,PORT_V37_IRP_SIZE,
                    (uint32_t)(uintptr_t)irp,
                    executive_devicedb_result.ucb[0],PORT_DQ_READPBLK)!=PORT_V37_QIO_OK)
                    return PORT_SYS_EXEC_BAD_PARAMETERS;
                port_v37_write16(irp+42u,PORT_DQ_PHYSIO);
                port_v37_write16(irp+50u,PORT_DQ_BOOTREAD_BYTES);
                port_v37_write32(irp+PORT_V37_IRP_MEDIA,lbn);
                struct port_dq_bootread_request aux_req=dq;
                aux_req.irp=irp;aux_req.sector=executive_dq_aux_sector.words;
                if(port_dq_bootread_execute_physical(&aux_req,&executive_dq_aux_io,lbn)
                        !=PORT_DQ_BOOTREAD_OK||
                   executive_dq_aux_io.ucb_io_count!=++completed)
                    return PORT_SYS_EXEC_BAD_PARAMETERS;
                const uint8_t *disk=(const uint8_t *)executive_dq_aux_sector.words;
                if(step==0u){
                    if(port_dq_image_verify_fh2(disk,&fid)!=PORT_DQ_IMAGE_OK||
                       port_dq_image_first_data(disk,&data_lbn)!=PORT_DQ_IMAGE_OK)
                        return PORT_SYS_EXEC_BAD_PARAMETERS;
                    for(unsigned k=0u;k<512u;k++) executive_dq_aux_header[k]=disk[k];
                }else{
                    for(unsigned k=0u;k<512u;k++) executive_dq_aux_elf[k]=disk[k];
                }
            }
            struct port_dq_aux_segment seg={0};
            if(port_dq_aux_descriptor(executive_dq_aux_elf,
                aux_files[file].start,aux_files[file].end,&seg)!=PORT_DQ_AUX_OK)
                return PORT_SYS_EXEC_BAD_PARAMETERS;
            for(uint32_t n=0u;n<seg.sectors;n++){
                uint32_t lbn=0u,va=0u,bytes=0u;
                if(port_dq_aux_sector(executive_dq_aux_header,&seg,n,
                       &lbn,&va,&bytes)!=PORT_DQ_AUX_OK||!bytes||bytes>512u)
                    return PORT_SYS_EXEC_BAD_PARAMETERS;
                uint8_t *irp=executive_dq_aux_irp.bytes;
                if(port_v37_irp_prepare(irp,PORT_V37_IRP_SIZE,
                    (uint32_t)(uintptr_t)irp,
                    executive_devicedb_result.ucb[0],PORT_DQ_READPBLK)!=PORT_V37_QIO_OK)
                    return PORT_SYS_EXEC_BAD_PARAMETERS;
                port_v37_write16(irp+42u,PORT_DQ_PHYSIO);
                port_v37_write16(irp+50u,PORT_DQ_BOOTREAD_BYTES);
                port_v37_write32(irp+PORT_V37_IRP_MEDIA,lbn);
                struct port_dq_bootread_request req=dq;
                req.irp=irp;req.sector=executive_dq_aux_sector.words;
                if(port_dq_bootread_execute_physical(&req,&executive_dq_aux_io,lbn)
                      !=PORT_DQ_BOOTREAD_OK||
                   executive_dq_aux_io.ucb_io_count!=++completed)
                    return PORT_SYS_EXEC_BAD_PARAMETERS;
                const uint8_t *disk=(const uint8_t *)executive_dq_aux_sector.words;
                const uint8_t *resident=(const uint8_t *)(uintptr_t)va;
                for(unsigned k=0u;k<bytes;k++)
                    if(disk[k]!=resident[k])return PORT_SYS_EXEC_BAD_PARAMETERS;
                executive_dq_aux_bytes+=bytes;
                executive_dq_aux_count++;
            }
        }
        if(executive_dq_aux_bytes<1500u||executive_dq_aux_count<4u)
            return PORT_SYS_EXEC_BAD_PARAMETERS;
        /* FIL$READVBN's unaligned byte range crosses three different
         * one-origin VBNs; all data is fetched through actual ATA IRPs.
         * The live DQA0 ACP and user QIO remain deliberately disabled. */
        struct executive_f11_context range={dq,completed,0u,
            executive_devicedb_result.ucb[0]};
        uint32_t range_done=0u;
        if(port_f11_read_range((const uint8_t *)executive_dq_image_header.words,
            text_offset+123u,1024u,executive_f11_range_result,
            sizeof executive_f11_range_result,executive_f11_native_sector,
            &range,&range_done)!=PORT_F11_RANGE_OK||
            range_done!=1024u||range.reads!=3u)
            return PORT_SYS_EXEC_BAD_PARAMETERS;
        for(unsigned i=0u;i<1024u;i++)
            if(executive_f11_range_result[i]!=
               ((const uint8_t *)(uintptr_t)(text_va+123u))[i])
                return PORT_SYS_EXEC_BAD_PARAMETERS;
        completed+=range.reads;
        /* Independent FIL$FINDFILID/RDCHKFILHDR/READVBN chain. It reopens
         * the actual SYS.EXE through MFD -> SYSEXE.DIR -> indexed FH2 and
         * rereads a nonaligned span of executable text from physical ATA.
         * No cached directory bytes, channel, ACP or invented login. */
        struct executive_f11_context pathctx={dq,completed,0u,
            executive_devicedb_result.ucb[0]};
        const struct port_f11_boot_volume volume={
            (const uint8_t *)executive_dq_boot_sector.words,
            (const uint8_t *)executive_dq_index_sector.words,
            (const uint8_t *)executive_dq_mfd_sector.words,
            executive_f11_native_sector,&pathctx,1u};
        uint32_t path_done=0u;
        uint32_t path_open_status=port_f11_boot_open(&volume,"SYSEXE.DIR",10u,
             "SYS.EXE",7u,1u,&executive_f11_opened_sys);
        if(path_open_status!=PORT_F11_PATH_OK){
            p->result->signature=UINT32_C(0xb3000000)|
                ((pathctx.previous_count&0xffu)<<16)|
                (executive_f11_range_io.ucb_io_count&0xffffu);
            return PORT_SYS_EXEC_BAD_PARAMETERS;
        }
        if(
           executive_f11_opened_sys.file.number!=11u||
           executive_f11_opened_sys.file.sequence!=11u||
           executive_f11_opened_sys.file_header_lbn!=image_header_lbn||
           port_f11_boot_read(&volume,&executive_f11_opened_sys,
                text_offset+219u,768u,executive_f11_opened_bytes,
                sizeof executive_f11_opened_bytes,&path_done)!=PORT_F11_PATH_OK||
           path_done!=768u||pathctx.reads!=6u)
            return PORT_SYS_EXEC_BAD_PARAMETERS;
        for(unsigned i=0u;i<768u;i++)
            if(executive_f11_opened_bytes[i]!=
                ((const uint8_t *)(uintptr_t)(text_va+219u))[i])
                return PORT_SYS_EXEC_BAD_PARAMETERS;
        completed+=pathctx.reads;
        /* Enumerate the real level-2 SYSEXE directory from ATA, not the
         * cached byte array. Only three validated image FIDs are needed
         * for this source INIT stage; no RMS wildcard lookup is claimed. */
        struct executive_f11_context listctx={dq,completed,0u,
            executive_devicedb_result.ucb[0]};
        struct port_f11_boot_volume list_volume=volume;
        list_volume.context=&listctx;
        struct executive_dir_listing list={0u,0u};
        uint32_t seen=0u;
        if(port_f11_bootdir_walk(&list_volume,
            (const uint8_t *)executive_dq_dir_sysexe_header.words,
            executive_source_directory_entry,&list,&seen)!=PORT_F11_BOOTDIR_OK||
           seen<3u||list.entries!=seen||list.mask!=7u||listctx.reads!=1u)
            return PORT_SYS_EXEC_BAD_PARAMETERS;
        completed+=listctx.reads;
        /* Source FIL$RDCHKFILHDR collects a checksummed FH2 extent chain.
         * Reuse SYSBOOT's normalized retrieval decoder, then confirm its
         * source physical mapping by independent native ATA file bytes. */
        struct executive_f11_context chainctx={dq,completed,0u,
            executive_devicedb_result.ucb[0]};
        struct port_f11_boot_volume chain_volume=volume;
        chain_volume.context=&chainctx;
        uint32_t chain_done=0u;
        if(port_f11_chain_collect(&chain_volume,
             &executive_f11_opened_sys.file,&executive_f11_chain)!=PORT_F11_CHAIN_OK||
           executive_f11_chain.headers!=1u||
           executive_f11_chain.blocks<text_sectors+8u||
           port_f11_chain_read(&chain_volume,&executive_f11_chain,
             text_offset+321u,777u,executive_f11_chain_text,
             sizeof executive_f11_chain_text,&chain_done)!=PORT_F11_CHAIN_OK||
           chain_done!=777u||chainctx.reads!=4u)
            return PORT_SYS_EXEC_BAD_PARAMETERS;
        for(unsigned i=0u;i<777u;i++)
            if(executive_f11_chain_text[i]!=
                ((const uint8_t *)(uintptr_t)(text_va+321u))[i])
                return PORT_SYS_EXEC_BAD_PARAMETERS;
        completed+=chainctx.reads;
        /* M153-M160: bounded FIL$READVBN persistent byte cursor over the
         * same source FH2. Real ATA requests, not a VMS process ACP/QIO.
         * Compare against the independently loaded resident SYS.EXE code. */
        struct executive_f11_context cursorctx={dq,completed,0u,
            executive_devicedb_result.ucb[0]};
        struct port_f11_boot_volume cursor_volume=volume;
        cursor_volume.context=&cursorctx;
        struct port_f11_cursor cursor={0};
        uint32_t cursor_matched=0u;
        if(port_f11_cursor_init(&cursor,&cursor_volume,&executive_f11_chain,
                    0u)!=PORT_F11_CURSOR_OK||
           port_f11_cursor_seek(&cursor,text_offset+400u)!=PORT_F11_CURSOR_OK||
           port_f11_cursor_compare(&cursor,
                (const uint8_t *)(uintptr_t)(text_va+400u),901u,
                &cursor_matched)!=PORT_F11_CURSOR_OK||
           cursor_matched!=901u||cursor.position!=text_offset+1301u||
           cursor.reads!=4u||cursorctx.reads!=4u)
            return PORT_SYS_EXEC_BAD_PARAMETERS;
        completed+=cursorctx.reads;
        /* M161-M168: source DQ_STARTIO and IOCIOPOST queue ownership,
         * actual read callback, exact FIFO, and completion checks over
         * three separate real Files-11 sectors of loaded SYS.EXE text.
         * This is not a live DQA0 DDT nor user $QIO/ACP. */
        struct executive_f11_context iocctx={dq,completed,0u,
            executive_devicedb_result.ucb[0]};
        if(port_dq_ioc_init(&executive_dq_private_ioc,
             executive_f11_native_sector,&iocctx)!=PORT_DQ_IOC_OK)
            return PORT_SYS_EXEC_BAD_PARAMETERS;
        for(uint32_t i=0;i<3u;i++){
            uint32_t lbn=0u;
            if(port_f11_chain_lbn(&executive_f11_chain,
                 (text_offset>>9)+1u+i,&lbn)!=PORT_F11_CHAIN_OK||
               port_dq_ioc_prepare(&executive_dq_private_ioc,
                 0xd100u+i,lbn,executive_ioc_private_sectors[i],512u)
                    !=PORT_DQ_IOC_OK||
               port_dq_ioc_set_retry(&executive_dq_private_ioc,
                   0xd100u+i,2u)!=PORT_DQ_IOC_OK||
               port_dq_ioc_enqueue(&executive_dq_private_ioc,0xd100u+i)
                    !=PORT_DQ_IOC_OK)
                return PORT_SYS_EXEC_BAD_PARAMETERS;
        }
        if(port_dq_ioc_validate(&executive_dq_private_ioc)!=PORT_DQ_IOC_OK)
            return PORT_SYS_EXEC_BAD_PARAMETERS;
        /* M169-M176: bounded RETRYERR -> FUNCXT -> IOCIOPOST.
         * Device reads still use actual ATA callback, not a public DDT.
         * The host injection tests exercise transient and permanent faults;
         * this guest path must not manufacture an ATA error. */
        struct port_dq_ioc_report iocreport={0};
        if(port_dq_ioc_run(&executive_dq_private_ioc,&iocreport)
              !=PORT_DQ_IOC_OK||iocreport.processed!=3u||
           iocreport.succeeded!=3u||iocreport.failed||iocreport.reads!=3u)
            return PORT_SYS_EXEC_BAD_PARAMETERS;
        for(uint32_t i=0;i<3u;i++){
            for(uint32_t j=0;j<512u;j++)
                if(executive_ioc_private_sectors[i][j]!=
                      ((const uint8_t *)(uintptr_t)(text_va+i*512u))[j])
                    return PORT_SYS_EXEC_BAD_PARAMETERS;
        }
        if(port_dq_ioc_validate(&executive_dq_private_ioc)!=PORT_DQ_IOC_OK||
           executive_dq_private_ioc.completed!=3u||
           executive_dq_private_ioc.failed!=0u||
           executive_dq_private_ioc.reads!=3u||iocctx.reads!=3u||
           executive_dq_private_ioc.posted!=3u||
           executive_dq_private_ioc.active!=PORT_DQ_IOC_DEPTH)
            return PORT_SYS_EXEC_BAD_PARAMETERS;
        completed+=iocctx.reads;
        /* M177-M184: private FILE -> IOC -> native ATA -> IOCIOPOST
         * boundary, mapping a fragment through original FH2 extents.
         * Independently match the actual resident SYS.EXE text, not fake
         * file bytes; active DQA0, ACP and process QIO remain absent. */
        struct executive_f11_context bridge_ctx={dq,completed,0u,
            executive_devicedb_result.ucb[0]};
        struct port_dq_ioc_queue *bridge_queue=&executive_f11_ioc_queue;
        struct port_f11_ioc_file *bridge_file=&executive_f11_ioc_file;
        uint32_t bridge_equal=0u;
        if(port_dq_ioc_init(bridge_queue,
             executive_f11_native_sector,&bridge_ctx)!=PORT_DQ_IOC_OK||
           port_f11_ioc_init(bridge_file,&executive_f11_chain,
             bridge_queue,executive_f11_chain.blocks*512u)
                !=PORT_F11_IOC_OK||
           port_f11_ioc_seek(bridge_file,text_offset+2100u)
                !=PORT_F11_IOC_OK||
           port_f11_ioc_compare(bridge_file,
             (const uint8_t *)(uintptr_t)(text_va+2100u),700u,
             &bridge_equal)!=PORT_F11_IOC_OK||bridge_equal!=700u||
           bridge_file->reads!=2u||bridge_ctx.reads!=2u||
           bridge_queue->completed!=2u||bridge_queue->posted!=2u||
           port_f11_ioc_validate(bridge_file)!=PORT_F11_IOC_OK)
            return PORT_SYS_EXEC_BAD_PARAMETERS;
        completed+=bridge_ctx.reads;
        /* M185-M192: persistent owner-scoped channel (not public $ASSIGN).
         * Read genuine SYS.EXE text through a second private IOC context;
         * preserve original FH2/VBN map and source DQ ATA completion. */
        struct executive_f11_context channel_ctx={dq,completed,0u,
            executive_devicedb_result.ucb[0]};
        uint32_t channel_handle=0u,channel_equal=0u;
        struct port_f11_chan_status channel_state={0};
        if(port_dq_ioc_init(&executive_f11_channel_ioc_queue,
             executive_f11_native_sector,&channel_ctx)!=PORT_DQ_IOC_OK||
           port_f11_chan_init(&executive_f11_channel_table,
             &executive_f11_chain,&executive_f11_channel_ioc_queue,
             executive_f11_chain.blocks*512u)!=PORT_F11_CHAN_OK||
           port_f11_chan_assign(&executive_f11_channel_table,1u,
             &channel_handle)!=PORT_F11_CHAN_OK||
           port_f11_chan_seek(&executive_f11_channel_table,1u,
             channel_handle,text_offset+2900u)!=PORT_F11_CHAN_OK||
           port_f11_chan_compare(&executive_f11_channel_table,1u,
             channel_handle,(const uint8_t *)(uintptr_t)(text_va+2900u),
             600u,&channel_equal)!=PORT_F11_CHAN_OK||
           channel_equal!=600u||channel_ctx.reads!=2u||
           port_f11_chan_status(&executive_f11_channel_table,1u,
             channel_handle,&channel_state)!=PORT_F11_CHAN_OK||
           channel_state.position!=text_offset+3500u||
           channel_state.physical_reads!=2u||
           port_f11_chan_release(&executive_f11_channel_table,1u,
             channel_handle)!=PORT_F11_CHAN_OK||
           executive_f11_channel_table.open_count!=0u||
           port_f11_chan_validate(&executive_f11_channel_table)
             !=PORT_F11_CHAN_OK)
            return PORT_SYS_EXEC_BAD_PARAMETERS;
        completed+=channel_ctx.reads;
        executive_f11_bootpath_ready=1u;
        executive_dq_image_probe=(struct port_dq_image_result){
           .header_lbn=image_header_lbn,.data_lbn=image_data_lbn,
           .entry=elf_entry,.irp_count=completed,
           .text_lbn=text_lbn,.text_va=text_va,
           .text_file_offset=text_offset,.text_bytes_verified=text_elf_filesz,
           .text_sectors_verified=text_sectors,.text_elf_filesz=text_elf_filesz,
           .aux_bytes_verified=executive_dq_aux_bytes,
           .aux_sectors_verified=executive_dq_aux_count,
           .range_bytes_verified=range_done,.range_irps=range.reads,
           .path_bytes_verified=path_done,
           .path_irps=pathctx.reads+listctx.reads,
           .path_entries_verified=seen,
           .chain_bytes_verified=chain_done,.chain_irps=chainctx.reads,
           .chain_headers_verified=executive_f11_chain.headers,
           .cursor_bytes_verified=cursor_matched,.cursor_irps=cursor.reads,
           .private_ioc_reads=iocctx.reads,.private_ioc_bytes=1536u,
           .f11_ioc_reads=bridge_ctx.reads,.f11_ioc_bytes=bridge_equal,
           .f11_channel_reads=channel_ctx.reads,.f11_channel_bytes=channel_equal,
           .file_number=executive_sysexecutable_fid.number,
           .file_sequence=executive_sysexecutable_fid.sequence,
           .elf_segments=elf_segments,.elf_type=2u};
        executive_dq_image_ready=1u;
        executive_dq_dir_ready=1u;
        executive_dq_mfd_ready=1u;
        executive_dq_index_ready=1u;
        executive_dq_bootzero_ready=1u;
        executive_dq_bootread_ready=1u;
        p->result->dq_image_probe=&executive_dq_image_probe;
        p->result->dq_sysexe_dir_fid=&executive_sysexe_dir_fid;
        p->result->dq_sysexecutable_fid=&executive_sysexecutable_fid;
        p->result->dq_dir_irp_last_count=executive_dq_dir_io[2].ucb_io_count;
        p->result->dq_mfd_header=&executive_dq_mfd_result;
        p->result->dq_mfd_sector=executive_dq_mfd_sector.words;
        p->result->dq_index_header=&executive_dq_index_result;
        p->result->dq_index_sector=executive_dq_index_sector.words;
        p->result->dq_bootzero=&executive_dq_bootzero_result;
        p->result->dq_bootzero_sector=executive_dq_bootzero_sector.words;
        p->result->dq_bootread=&executive_dq_bootread_result;
        p->result->dq_bootread_sector=executive_dq_boot_sector.words;
        p->result->signature=PORT_SYS_EXEC_DQ_BOOTREAD_TAG;
        return PORT_SYS_EXEC_NORMAL;
    }
    if(p->phase==PORT_SYS_EXEC_INIT_FIN_DATABASE){
        /* Original SYS/SRC/INIT.MAR FIX_DRV_NAME and FIN_DATABASE:
         * source DDB$T_NAME/DRVNAME, the RPB unit and ASCIC LOG records.
         * Must not publish logical names or claim that a DQ DDT/ACP exists.
         * This stage is gated by one *real* verified ATA Files-11 read. */
        if(!executive_dq_bootread_ready||!executive_dq_bootzero_ready||
           !executive_dq_index_ready||!executive_dq_mfd_ready||
           !executive_dq_dir_ready||!executive_dq_image_ready||
           executive_dq_bootzero_result.lbn!=0u||
           executive_dq_bootzero_result.ucb_io_count!=2u||
           !executive_devicedb_ready||
           !executive_dq_fdt_ready||!executive_syslog_ready||
           !executive_nl_ddt_ready||!executive_bootdev_scan_ready||
           executive_fin_database_ready||
           executive_bootdev_scan.boot_kind!=PORT_INI_PC_ATA_PIO||
           executive_dq_bootread_result.bytes!=PORT_DQ_BOOTREAD_BYTES||
           executive_dq_bootread_result.lbn!=PORT_DQ_BOOTREAD_LBN||
           executive_dq_bootread_result.irp_status!=1u||
           executive_dq_bootread_result.unit!=0u||
           executive_devicedb_result.boot_name_ready!=0u||
           executive_devicedb_result.incomplete_driver_mask!=7u||
           !executive_syslog.records[0]||!executive_syslog.records[1]||
           executive_syslog.logical_table[0]!=(uint32_t)(uintptr_t)executive_syslog.records[0]||
           executive_syslog.logical_table[1]!=(uint32_t)(uintptr_t)executive_syslog.records[1])
            return PORT_SYS_EXEC_BAD_PARAMETERS;
        struct port_ini_fin_settings settings={
            .device_bytes=executive_devicedb_bytes.data,
            .device_size=sizeof executive_devicedb_bytes.data,
            .device_base=(uint32_t)(uintptr_t)executive_devicedb_bytes.data,
            .device_info=&executive_devicedb_result,
            .disk_log=(uint8_t *)(uintptr_t)executive_syslog.records[0],
            .sysdevice_log=(uint8_t *)(uintptr_t)executive_syslog.records[1],
            .disk_log_size=executive_syslog.sizes[0],
            .sysdevice_log_size=executive_syslog.sizes[1],
            .unit=0u,.concealed=1u,.adapter_is_ata=1u};
        if(port_ini_fin_database(&settings,&executive_fin_database_result)!=PORT_FINDB_OK||
           port_ini_fin_check(&settings,&executive_fin_database_result)!=PORT_FINDB_OK||
           port_v37_read32(executive_devicedb_bytes.data+12u)!=0u)
            return PORT_SYS_EXEC_BAD_PARAMETERS;
        executive_fin_database_ready=1u;
        p->result->boot_device_names=&executive_fin_database_result;
        p->result->device_db=&executive_devicedb_result;
        p->result->device_db_bytes=executive_devicedb_bytes.data;
        p->result->syslog=&executive_syslog;
        p->result->signature=PORT_SYS_EXEC_FIN_DATABASE_TAG;
        return PORT_SYS_EXEC_NORMAL;
    }
    if(p->phase==PORT_SYS_EXEC_INIT_SYSTEM_LOG){
        /* LOGNAMSUB.MAR LOG$HASH/INSLOGN/SEARCHLOG/TRANSLATE stage:
         * original source counters and 32-bit native record links, but only
         * the two verified FIN_DATABASE system bootstrap names are safe.
         * The real multi-table $TRNLOG, mutex and process/group tables
         * are still absent, and no DQ disk driver is bound. */
        if(!executive_fin_database_ready||!executive_syslog_ready||
           !executive_devicedb_ready||!executive_dq_bootread_ready||
           executive_boot_system_log_ready||
           executive_devicedb_result.boot_name_ready!=1u||
           executive_devicedb_result.incomplete_driver_mask!=7u||
           executive_syslog.sizes[0]!=PORT_LOG_REC_LEN||
           executive_syslog.sizes[1]!=PORT_LOG_REC_LEN||
           !executive_syslog.records[0]||!executive_syslog.records[1])
            return PORT_SYS_EXEC_BAD_PARAMETERS;
        if(port_log_system_init(&executive_boot_system_log,
               (uint8_t *)(uintptr_t)executive_syslog.records[0],
               (uint8_t *)(uintptr_t)executive_syslog.records[1],
               PORT_LOG_REC_LEN,PORT_LOG_BITS)!=PORT_LOG_OK||
           port_log_system_check(&executive_boot_system_log)!=PORT_LOG_OK)
            return PORT_SYS_EXEC_BAD_PARAMETERS;
        uint8_t translated[16]={0};struct port_log_result result={0};
        if(port_log_translate(&executive_boot_system_log,
               "SYS$DISK",8u,0u,translated,sizeof translated,&result)!=PORT_LOG_OK||
           result.bytes!=6u||result.record_address!=
               executive_boot_system_log.record_addr[0]||
           translated[0]!='_'||translated[1]!='D'||
           translated[2]!='Q'||translated[3]!='A'||
           translated[4]!='0'||translated[5]!=':'||
           port_log_translate(&executive_boot_system_log,
               "SYS$SYSDEVICE",13u,0u,translated,sizeof translated,
               &result)!=PORT_LOG_OK||
           result.bytes!=7u||result.record_address!=
               executive_boot_system_log.record_addr[1]||
           translated[0]!='_'||translated[1]!='_'||translated[2]!='D'||
           translated[3]!='Q'||translated[4]!='A'||
           translated[5]!='0'||translated[6]!=':'||
           port_log_translate(&executive_boot_system_log,
               "_DQA0:",6u,0u,translated,sizeof translated,
               &result)!=PORT_LOG_NOTRAN||
           result.bytes!=5u||translated[0]!='D'||translated[1]!='Q'||
           translated[2]!='A'||translated[3]!='0'||translated[4]!=':')
            return PORT_SYS_EXEC_BAD_PARAMETERS;
        executive_boot_system_log_ready=1u;
        p->result->boot_system_log_table=&executive_boot_system_log;
        p->result->boot_system_translate=port_log_translate;
        p->result->signature=PORT_SYS_EXEC_SYSTEM_LOG_TAG;
        return PORT_SYS_EXEC_NORMAL;
    }
    if(p->phase==PORT_SYS_EXEC_INIT_IOPOST_END){
        /* Wire the source IOCIOPOST.MAR entry to supervisor x86 IDT 0x90.
         * Bootstrap may NOT drain the FIFO on its normal function stack. */
        const uintptr_t address=(uintptr_t)executive_iopost_irq_entry;
        const uintptr_t kernel_end=(uintptr_t)port_v37_iopost_kernel_end;
        if(!executive_post_ready||!executive_nl_ddt_ready||
           executive_iopost_end_ready||executive_iopost_irq_armed||
           executive_post_queue.queued!=2u||
           executive_post_soft_level!=PORT_V37_IOPOST_IPL||
           executive_post_soft_count!=2u||
           kernel_end<PORT_SYS_ELF_START||kernel_end>=PORT_SYS_ELF_LIMIT||
           address<PORT_SYS_ELF_START||address>=PORT_SYS_ELF_LIMIT)
            return PORT_SYS_EXEC_BAD_PARAMETERS;
        executive_iopost_irq_armed=1u;
        p->result->iopost_isr=executive_iopost_irq_entry;
        p->result->iopost_remaining=executive_post_queue.queued;
        p->result->iopost=&executive_post_queue;
        p->result->signature=PORT_SYS_EXEC_IOPOST_BIND_TAG;
        return PORT_SYS_EXEC_NORMAL;
    }
    if(p->phase==PORT_SYS_EXEC_VERIFY_IOPOST_INT){
        /* Proof that the IDT handler really invoked SYS.EXE; until
         * this phase, there is no recorded IOC completion success. */
        if(!executive_iopost_end_ready||executive_iopost_irq_armed||
           executive_post_queue.queued!=0u||
           executive_iopost_completed.completed!=2u||
           executive_iopost_completed.status[0]!=PORT_SYS85_ENDOFFILE||
           executive_iopost_completed.status[1]!=PORT_SYS85_NORMAL||
           executive_iopost_completed.secondary[0]!=0u||
           executive_iopost_completed.secondary[1]!=0u)
            return PORT_SYS_EXEC_BAD_PARAMETERS;
        p->result->iopost_endlog=&executive_iopost_completed;
        p->result->iopost_remaining=executive_post_queue.queued;
        p->result->iopost=&executive_post_queue;
        p->result->signature=PORT_SYS_EXEC_IOPOST_END_TAG;
        return PORT_SYS_EXEC_NORMAL;
    }
    if(p->phase==PORT_SYS_EXEC_INIT_QIO_POST){
        /* NLDRIVER.MAR READ/WRITE -> SYSQIOREQ.MAR 741-749 EXE$FINISHIOC:
         * two correctly linked IRP status packets in resident SYS.EXE,
         * queue insertion and pending IPL$_IOPOST=4. No post-processing
         * interrupt dispatch, channel allocation or login exists yet. */
        if(!executive_ioc_ready || !executive_devicedb_ready ||
           executive_post_ready || !executive_nl_ddt_ready ||
           executive_devicedb_result.incomplete_driver_mask!=7u ||
           (uintptr_t)executive_post_irps.bytes>UINT32_MAX ||
           (uintptr_t)&executive_post_queue.first>UINT32_MAX)
            return PORT_SYS_EXEC_BAD_PARAMETERS;
        uint8_t *const u=executive_devicedb_bytes.data+PORT_V37_UCB_NL_OFF;
        const uint32_t base=(uint32_t)(uintptr_t)executive_post_irps.bytes;
        const uint32_t ucb=executive_devicedb_result.ucb[4];
        if(port_v37_post_init(&executive_post_queue,executive_post_irps.bytes,
              sizeof executive_post_irps.bytes,base,PORT_V37_IRP_SIZE,
              (uint32_t)(uintptr_t)&executive_post_queue.first)!=PORT_V37_QIO_OK)
            return PORT_SYS_EXEC_BAD_PARAMETERS;
        const uint16_t functions[2]={PORT_SYS85_READVBLK,PORT_SYS85_WRITELBLK};
        const uint32_t outcomes[2]={PORT_SYS85_ENDOFFILE,PORT_SYS85_NORMAL};
        for(unsigned i=0;i<2u;i++){
            uint8_t *irp=executive_post_irps.bytes+i*PORT_V37_IRP_SIZE;
            if(port_v37_irp_prepare(irp,PORT_V37_IRP_SIZE,
                    base+i*PORT_V37_IRP_SIZE,ucb,functions[i])!=PORT_V37_QIO_OK)
                return PORT_SYS_EXEC_BAD_PARAMETERS;
            const uintptr_t kernel_end=(uintptr_t)port_v37_iopost_kernel_end;
            if(kernel_end<PORT_SYS_ELF_START || kernel_end>=PORT_SYS_ELF_LIMIT)
                return PORT_SYS_EXEC_BAD_PARAMETERS;
            /* IOCIOPOST interprets IRP$L_PID as a privileged end-action
             * address when there is no process PID. Unlike the VAX negative
             * address convention, this is a protected native SYS.EXE PC. */
            port_v37_write32(irp+PORT_V37_IRP_PID,(uint32_t)kernel_end);
            struct port_v37_qio_request req={
                .post=&executive_post_queue,.irp=irp,.ucb=u,
                .irp_va=base+i*PORT_V37_IRP_SIZE,.ucb_va=ucb,
                .nl_ddb_va=executive_devicedb_result.ddb[3],
                .irp_size=PORT_V37_IRP_SIZE,.ucb_size=PORT_V37_UCB_SIZE,
                .softint=executive_post_mark_softint};
            if(port_v37_nl_ddt_dispatch(executive_nl_ddt.bytes,
                   sizeof executive_nl_ddt.bytes,
                   (uint32_t)(uintptr_t)executive_nl_ddt.bytes,
                   (uint32_t)(uintptr_t)port_v37_nl_ioc_return,
                   (uint32_t)(uintptr_t)port_v37_nl_fdt_read,
                   (uint32_t)(uintptr_t)port_v37_nl_fdt_write,
                   functions[i],&req,port_v37_nl_fdt_read,
                   port_v37_nl_fdt_write)!=PORT_SYS85_NORMAL ||
               port_v37_read32(irp+PORT_V37_IRP_MEDIA)!=outcomes[i] ||
               port_v37_read32(irp+PORT_V37_IRP_IOST2)!=0u ||
               executive_post_soft_count!=i+1u)
                return PORT_SYS_EXEC_BAD_PARAMETERS;
        }
        if(port_v37_post_validate(&executive_post_queue)!=PORT_V37_QIO_OK ||
           executive_post_queue.first!=base ||
           executive_post_queue.last!=base+PORT_V37_IRP_SIZE ||
           executive_post_queue.queued!=2u ||
           port_v37_read32(executive_post_irps.bytes)!=base+PORT_V37_IRP_SIZE ||
           port_v37_read32(executive_post_irps.bytes+PORT_V37_IRP_SIZE)!=
                 executive_post_queue.sentinel ||
           port_v37_read32(u+PORT_V37_UCB_OPCNT)!=2u ||
           executive_post_soft_level!=PORT_V37_IOPOST_IPL)
            return PORT_SYS_EXEC_BAD_PARAMETERS;
        executive_post_ready=1u;
        p->result->iopost=&executive_post_queue;
        p->result->post_irp_bytes=executive_post_irps.bytes;
        p->result->iopost_softint_count=executive_post_soft_count;
        p->result->iopost_softint_level=executive_post_soft_level;
        p->result->iopost_ucb_ops=port_v37_read32(u+PORT_V37_UCB_OPCNT);
        p->result->nl_post=port_v37_nl_fdt_post;
        p->result->signature=PORT_SYS_EXEC_POST_RESULT_TAG;
        return PORT_SYS_EXEC_NORMAL;
    }
    if(p->phase==PORT_SYS_EXEC_INIT_DQ_DDT_STAGE){
        /* DQDRIVER.MAR 392-465, 496-592; SHRLIB/SYSMAR.MAR DDTAB.
         * Stage only. This is not a bound PC disk DDT or an ACP/QIO. */
        if(!executive_ioc_logical_ready||!executive_dq_fdt_ready||
           !executive_dq_bootread_ready||!executive_fin_database_ready||
           !executive_boot_system_log_ready||executive_dqddt_ready||
           executive_devicedb_result.incomplete_driver_mask!=7u||
           !p->dq_boot_io||!p->dq_boot_io->read_sector||
           !p->dq_boot_io->irq_handler||
           p->dq_boot_io->boot_unit!=0u||p->dq_boot_io->reserved||
           port_v37_read32(executive_devicedb_bytes.data+
               PORT_V37_DDB_BOOT_OFF+12u)!=0u||
           port_v37_read32(executive_devicedb_bytes.data+
               PORT_V37_UCB_BOOT_OFF+120u)!=0u)
            return PORT_SYS_EXEC_BAD_PARAMETERS;
        const uint32_t base=(uint32_t)(uintptr_t)executive_dqddt.image;
        const uint32_t start=(uint32_t)(uintptr_t)port_dqddt_startio;
        const uint32_t ret=(uint32_t)(uintptr_t)port_v37_nl_ioc_return;
        const uint32_t read_action=(uint32_t)(uintptr_t)port_dqddt_bootread;
        const uint32_t write_action=(uint32_t)(uintptr_t)port_dqddt_bootwrite;
        if(port_dqddt_stage_init(&executive_dqddt,&executive_dq_fdt,
               base,start,ret,read_action,write_action)!=PORT_DQDDT_OK||
           port_dqddt_validate(&executive_dqddt,&executive_dq_fdt)!=
               PORT_DQDDT_OK||
           port_dqddt_function(&executive_dqddt,&executive_dq_fdt,
               PORT_DQ_READPBLK)!=PORT_DQDDT_NATIVE||
           port_dqddt_function(&executive_dqddt,&executive_dq_fdt,
               PORT_DQ_WRITEPBLK)!=PORT_DQDDT_NATIVE||
           port_dqddt_function(&executive_dqddt,&executive_dq_fdt,
               PORT_DQ_WRITELBLK)!=PORT_DQDDT_UNBOUND||
           port_dqddt_function(&executive_dqddt,&executive_dq_fdt,
               63u)!=PORT_DQDDT_BAD||
           port_v37_read32(executive_devicedb_bytes.data+
               PORT_V37_DDB_BOOT_OFF+12u)!=0u||
           port_v37_read32(executive_devicedb_bytes.data+
               PORT_V37_UCB_BOOT_OFF+120u)!=0u)
            return PORT_SYS_EXEC_BAD_PARAMETERS;
        if(port_dq_crb_init(&executive_dq_crb,p->dq_boot_io->irq_handler,
               PORT_DQ_CRB_IRQ14_VECTOR)!=PORT_DQ_CRB_OK)
            return PORT_SYS_EXEC_BAD_PARAMETERS;
        executive_dqddt_ready=1u;
        p->result->dq_ddt_stage=&executive_dqddt;
        p->result->dq_ddt_function=port_dqddt_function;
        p->result->dq_ddt_dispatch_bootwrite=port_dqddt_dispatch_bootwrite;
        p->result->dq_ddt_startio=port_dqddt_startio;
        p->result->dq_ddt_startio_async=port_dqddt_startio_async;
        p->result->dq_ddt_startio_async_write=
            port_dqddt_startio_async_write;
        p->result->dq_bootread_commit=port_dq_bootread_commit;
        p->result->dq_bootwrite_commit=port_dq_bootwrite_commit;
        p->result->dq_crb=&executive_dq_crb;
        p->result->signature=PORT_SYS_EXEC_DQ_DDT_TAG;
        return PORT_SYS_EXEC_NORMAL;
    }
    if(p->phase==PORT_SYS_EXEC_INIT_DQ_CRB_READ){
        if(!executive_dqddt_ready||!executive_relocated_private_ready||
           !p->dq_boot_io||
           !p->dq_boot_io->read_sector||!p->dq_boot_io->irq_handler||
           port_dq_crb_validate(&executive_dq_crb)!=PORT_DQ_CRB_OK||
           executive_dq_crb_read_ready||
           port_v37_read32(executive_devicedb_bytes.data+
               PORT_V37_UCB_BOOT_OFF+32u)!=0u||
           port_v37_read32(executive_devicedb_bytes.data+
               PORT_V37_DDB_BOOT_OFF+12u)!=0u||
           port_v37_read32(executive_devicedb_bytes.data+
               PORT_V37_UCB_BOOT_OFF+120u)!=0u)
            return PORT_SYS_EXEC_BAD_PARAMETERS;
        port_v37_write32(executive_devicedb_bytes.data+
            PORT_V37_UCB_BOOT_OFF+32u,(uint32_t)(uintptr_t)&executive_dq_crb);
        executive_devicedb_result.bound_dqa_crb=
            (uint32_t)(uintptr_t)&executive_dq_crb;
        if(port_v37_devices_check(executive_devicedb_bytes.data,
              sizeof executive_devicedb_bytes.data,
              (uint32_t)(uintptr_t)executive_devicedb_bytes.data,
              &executive_devicedb_result)!=PORT_V37_DEV_OK)
            return PORT_SYS_EXEC_BAD_PARAMETERS;
        const uint32_t irp_free_before=executive_irps.free_count;
        uint8_t *allocated_irp=0;uint32_t allocated_irp_va=0u;
        if(executive_qio_alloc_irp(&allocated_irp,&allocated_irp_va)!=
               PORT_V37_IRP_POOL_OK||
           port_v37_irp_prepare(allocated_irp,PORT_V37_IRP_SIZE,
               allocated_irp_va,executive_devicedb_result.ucb[0],
               PORT_DQ_READPBLK)!=PORT_V37_QIO_OK){
            if(allocated_irp)(void)executive_qio_free_irp(allocated_irp,
                                                          allocated_irp_va);
            return PORT_SYS_EXEC_BAD_PARAMETERS;
        }
        port_v37_write16(allocated_irp+42u,PORT_DQ_PHYSIO);
        port_v37_write16(allocated_irp+50u,PORT_DQ_BOOTREAD_BYTES);
        port_v37_write32(allocated_irp+PORT_V37_IRP_MEDIA,0u);
        const struct port_dq_bootread_request read={
            .fdt=&executive_dq_fdt,.device_db=executive_devicedb_bytes.data,
            .device_info=&executive_devicedb_result,
            .irp=allocated_irp,.irp_bytes=PORT_V37_IRP_SIZE,
            .sector=executive_dqddt_sector.words,
            .sector_bytes=PORT_DQ_BOOTREAD_BYTES,
            .boot_unit=p->dq_boot_io->boot_unit,
            .read_sector=executive_dq_ioc_read_sector,
            .read_context=(void *)p->dq_boot_io};
        typedef uint32_t (*dq_startio_fn)(const struct port_dqddt_stage *,
            const struct port_dqddt_native *,const struct port_dq_fdt *,
            const struct port_dq_bootread_request *,
            struct port_dq_bootread_result *);
        const uint32_t start_pc=port_v37_read32(executive_native_dqddt.image);
        if(start_pc!=(uint32_t)(uintptr_t)port_dqddt_startio){
            (void)executive_qio_free_irp(allocated_irp,allocated_irp_va);
            return PORT_SYS_EXEC_BAD_PARAMETERS;
        }
        dq_startio_fn startio=(dq_startio_fn)(uintptr_t)start_pc;
        const uint32_t read_status=startio(&executive_dqddt,
               &executive_native_dqddt,&executive_dq_fdt,&read,
               &executive_dqddt_io);
        const uint32_t free_status=executive_qio_free_irp(allocated_irp,
                                                           allocated_irp_va);
        if(read_status!=PORT_DQ_BOOTREAD_OK||
           free_status!=PORT_V37_IRP_POOL_OK||
           executive_irps.free_count!=irp_free_before||
           executive_dqddt_io.lbn!=0u||
           executive_dqddt_io.bytes!=PORT_DQ_BOOTREAD_BYTES||
           executive_dqddt_io.irp_status!=PORT_SYS85_NORMAL||
           executive_dqddt_io.io_function!=PORT_DQ_READPBLK||
           executive_dqddt_io.ucb_media!=0u)
            return PORT_SYS_EXEC_BAD_PARAMETERS;
        executive_dq_crb_read_ready=1u;
        p->result->dq_crb=&executive_dq_crb;
        p->result->dq_ddt_bootread=&executive_dqddt_io;
        p->result->dq_ddt_bootread_sector=executive_dqddt_sector.words;
        p->result->signature=PORT_SYS_EXEC_DQ_CRB_READ_TAG;
        return PORT_SYS_EXEC_NORMAL;
    }
    if(p->phase==PORT_SYS_EXEC_INIT_DQ_DDT_PUBLISH){
        if(!executive_dq_crb_read_ready||!executive_dqddt_ready||
           !executive_relocated_private_ready||!executive_devicedb_ready||
           !executive_ioc_ready||executive_devicedb_result.incomplete_driver_mask!=7u)
            return PORT_SYS_EXEC_BAD_PARAMETERS;
        p->result->device_db=&executive_devicedb_result;
        p->result->device_db_bytes=executive_devicedb_bytes.data;
        p->result->dq_native_ddt=&executive_native_dqddt;
        p->result->ioc_search=port_v37_ioc_searchdev;
        p->result->dq_ddt_startio_async_write=
            port_dqddt_startio_async_write;
        p->result->dq_bootwrite_commit=port_dq_bootwrite_commit;
        p->result->dqa_ioc_status=port_dqddt_publish_ioc(executive_devicedb_bytes.data,
             sizeof executive_devicedb_bytes.data,
             (uint32_t)(uintptr_t)executive_devicedb_bytes.data,
             &executive_devicedb_result,&executive_dqddt,&executive_dq_fdt,
             &executive_native_dqddt,&executive_dq_crb);
        if(p->result->dqa_ioc_status!=PORT_DQDDT_BIND_OK){
            p->result->signature=PORT_SYS_EXEC_DQ_DDT_PUBLISH_TAG;
            return PORT_SYS_EXEC_NORMAL;
        }
        struct port_v37_ioc_result found={0};
        p->result->dqa_ioc_status=port_v37_ioc_searchdev(executive_devicedb_bytes.data,
             sizeof executive_devicedb_bytes.data,
             (uint32_t)(uintptr_t)executive_devicedb_bytes.data,
             &executive_devicedb_result,"DQA0",4u,&found);
        if(p->result->dqa_ioc_status!=PORT_V37_IOC_OK||
           found.ddb!=executive_devicedb_result.ddb[0]||
           found.ucb!=executive_devicedb_result.ucb[0]||found.unit!=0u||
           !found.driver_bound){
            p->result->signature=PORT_SYS_EXEC_DQ_DDT_PUBLISH_TAG;
            p->result->dqa_ioc_ddb=found.ddb;
            p->result->dqa_ioc_ucb=found.ucb;
            return PORT_SYS_EXEC_NORMAL;
        }
        if(!p->dq_boot_io||!p->dq_boot_io->read_sector||
           !p->dq_boot_io->irq_handler||!executive_dq_image_ready||
           !executive_f11_chain.headers)
            return PORT_SYS_EXEC_BAD_PARAMETERS;
        struct executive_postpub_f11_context f11ctx={p->dq_boot_io,0u};
        const struct port_f11_boot_volume f11volume={
            (const uint8_t *)executive_dq_boot_sector.words,
            (const uint8_t *)executive_dq_index_sector.words,
            (const uint8_t *)executive_dq_mfd_sector.words,
            executive_postpub_f11_sector,&f11ctx,1u};
        uint8_t chain_sample[700];
        uint32_t chain_done=0u;
        const uint32_t sample_offset=
            executive_dq_image_probe.text_file_offset+321u;
        if(sample_offset<executive_dq_image_probe.text_file_offset||
           port_f11_chain_read(&f11volume,&executive_f11_chain,
               sample_offset,sizeof chain_sample,chain_sample,
               sizeof chain_sample,&chain_done)!=PORT_F11_CHAIN_OK||
           chain_done!=sizeof chain_sample||f11ctx.reads!=2u)
            return PORT_SYS_EXEC_BAD_PARAMETERS;
        for(unsigned i=0u;i<sizeof chain_sample;i++)
            if(chain_sample[i]!=executive_f11_chain_text[i])
                return PORT_SYS_EXEC_BAD_PARAMETERS;
        /* Exercise the Files-11 ACP RDHEDR path for the just-opened SYS.EXE.
         * Its FID-to-VBN and FH2 retrieval-map callbacks read the same
         * published DQA0 DDT through the CRB IRQ14 completion path. */
        const uint8_t *home=(const uint8_t *)executive_dq_boot_sector.words;
        const uint8_t *index_header=
            (const uint8_t *)executive_dq_index_sector.words;
        const uint32_t bitmap_blocks=port_dq_index_word(home,32u);
        const uint32_t cluster=port_dq_index_word(home,14u);
        if(bitmap_blocks>UINT8_MAX||cluster>UINT16_MAX)
            return PORT_SYS_EXEC_BAD_PARAMETERS;
        struct executive_acp_io_context acp_io={{p->dq_boot_io,0u},
            index_header,executive_f11_opened_sys.file.volume};
        const struct port_f11_87_ops acp_ops={
            .context=&acp_io,.alloc=executive_acp_alloc,
            .dealloc=executive_acp_dealloc,
            .map_vbn=executive_acp_map_vbn,
            .read_lbn=executive_acp_read_lbn,
            .switch_volume=executive_acp_switch_volume};
        struct port_f11_87_volume acp_volume={0};
        acp_volume.index_bitmap_blocks=(uint8_t)bitmap_blocks;
        acp_volume.cluster=(uint16_t)cluster;
        acp_volume.current_rvn=executive_f11_opened_sys.file.volume;
        struct port_f11_87_context acp_context={
            .volume=&acp_volume,.ops=acp_ops};
        const struct port_f11_86_fid sys_fid={
            executive_f11_opened_sys.file.number,
            executive_f11_opened_sys.file.sequence,
            executive_f11_opened_sys.file.volume,0u};
        static uint8_t acp_sys_header[512] __attribute__((aligned(4)));
        uint32_t acp_header_lbn=0u;
        if(port_f11b_read_header(&acp_context,&sys_fid,0,
              acp_sys_header,&acp_header_lbn)!=PORT_F11_87_NORMAL||
           acp_header_lbn!=executive_f11_opened_sys.file_header_lbn||
           acp_io.disk.reads!=1u)
            return PORT_SYS_EXEC_BAD_PARAMETERS;
        for(unsigned i=0u;i<sizeof acp_sys_header;i++)
            if(acp_sys_header[i]!=executive_f11_opened_sys.header[i])
                return PORT_SYS_EXEC_BAD_PARAMETERS;
        executive_acp_allocation_source=UINT32_MAX;
        struct port_f11_87_fcb *acp_sys_fcb=
            port_f11b_create_fcb(&acp_context,acp_sys_header,
                                 acp_header_lbn);
        const uintptr_t pool_start=(uintptr_t)executive_np_pool.base;
        const uintptr_t pool_end=pool_start+executive_np_pool.bytes;
        const uintptr_t packet_start=(uintptr_t)executive_packet_pages;
        const uintptr_t packet_end=packet_start+sizeof executive_packet_pages;
        const int fcb_from_variable=
            executive_acp_allocation_source==PORT_EXE_NP_FROM_VARIABLE;
        const int fcb_from_lookaside=
            executive_acp_allocation_source==PORT_EXE_NP_FROM_IRP||
            executive_acp_allocation_source==PORT_EXE_NP_FROM_LRP||
            executive_acp_allocation_source==PORT_EXE_NP_FROM_SRP;
        if(!acp_sys_fcb||
           (uintptr_t)acp_sys_fcb>UINT32_MAX-sizeof(*acp_sys_fcb)+1u||
           (!fcb_from_variable&&!fcb_from_lookaside)||
           (fcb_from_variable&&
             ((uintptr_t)acp_sys_fcb<pool_start||
              (uintptr_t)acp_sys_fcb>pool_end-sizeof(*acp_sys_fcb)||
              executive_np_pool.allocations!=1u))||
           (fcb_from_lookaside&&
             ((uintptr_t)acp_sys_fcb<packet_start||
              (uintptr_t)acp_sys_fcb>packet_end-sizeof(*acp_sys_fcb)||
              executive_np_pool.allocations!=0u))||
           port_exe_np_pool_check(&executive_np_pool)!=PORT_EXE_NP_POOL_OK||
           acp_sys_fcb->header_lbn!=acp_header_lbn||
           acp_sys_fcb->fid.number!=sys_fid.number||
           acp_sys_fcb->fid.sequence!=sys_fid.sequence||
           acp_sys_fcb->fid.volume!=sys_fid.volume||
           acp_sys_fcb->file_blocks==0u)
            return PORT_SYS_EXEC_BAD_PARAMETERS;
        const uint32_t irp_free_before=executive_irps.free_count;
        const uint32_t lrp_free_before=executive_lrps.free_count;
        const uint32_t srp_free_before=executive_srps.free_count;
        const uint32_t pool_allocations_before=executive_np_pool.allocations;
        const uint32_t fcb_source=executive_acp_allocation_source;
        struct port_f11_87_window *acp_window=executive_acp_alloc(
            0u,sizeof(struct port_f11_87_window),PORT_F11_87_WCBTYPE);
        if(!acp_window)return PORT_SYS_EXEC_BAD_PARAMETERS;
        for(unsigned i=0u;i<sizeof(*acp_window);i++)
            ((uint8_t *)acp_window)[i]=0u;
        acp_window->fcb=acp_sys_fcb;
        acp_window->read=1u;
        acp_window->no_write=1u;
        acp_window->no_truncate=1u;
        acp_window->allocation_owned=1u;
        acp_window->allocation_base=acp_window;
        acp_window->allocation_bytes=sizeof(*acp_window);
        uint8_t acp_fib_input[PORT_F11_87_FIBLEN]={0};
        struct port_f11_87_fib_result acp_fib_result={0};
        struct port_f11_87_context fib_context=acp_context;
        fib_context.window=acp_window;
        if(port_f11b_get_fib(&fib_context,acp_fib_input,
              sizeof acp_fib_input,PORT_F11_87_IO_ACPCONTROL,
              &acp_fib_result)!=PORT_F11_87_NORMAL||
           acp_fib_result.length!=PORT_F11_87_FIBLEN||
           port_v37_read16(acp_fib_result.bytes+4u)!=sys_fid.number||
           port_v37_read16(acp_fib_result.bytes+6u)!=sys_fid.sequence||
           port_v37_read16(acp_fib_result.bytes+8u)!=sys_fid.volume||
           (acp_fib_result.bytes[23]&0x80u)||fib_context.cleanup_directory)
            return PORT_SYS_EXEC_BAD_PARAMETERS;
        p->result->dq_acp_fib_identified_sys_file=1u;
        struct port_f11_87_window *writeback=0;
        if(port_f11b_make_access(&acp_context,acp_sys_fcb,acp_window,
                                 &writeback)!=PORT_F11_87_NORMAL||
           writeback!=acp_window||acp_sys_fcb->accesses!=1u||
           acp_sys_fcb->no_writers!=1u||acp_sys_fcb->no_truncate!=1u||
           acp_context.open_count!=1u||acp_volume.transactions!=1u||
           !acp_context.cleanup_deaccess)
            return PORT_SYS_EXEC_BAD_PARAMETERS;
        acp_context.primary=acp_sys_fcb;
        acp_context.window=acp_window;
        const struct port_f11_87_deaccess_request readonly_close={0u,0u,0u,0u};
        if(port_f11b_deaccess(&acp_context,&readonly_close)!=
              PORT_F11_87_NORMAL||acp_sys_fcb->accesses||
           acp_sys_fcb->no_writers||acp_sys_fcb->no_truncate||
           acp_sys_fcb->windows||acp_sys_fcb->window_tail||
           acp_context.open_count||acp_volume.transactions||
           acp_context.primary||acp_context.window||
           acp_context.cleanup_deaccess||
           executive_acp_allocation_source!=fcb_source||
           executive_irps.free_count!=irp_free_before||
           executive_lrps.free_count!=lrp_free_before||
           executive_srps.free_count!=srp_free_before||
           executive_np_pool.allocations!=pool_allocations_before||
           port_exe_np_pool_check(&executive_np_pool)!=PORT_EXE_NP_POOL_OK)
            return PORT_SYS_EXEC_BAD_PARAMETERS;
        executive_acp_allocation_source=fcb_source;
        p->result->dq_acp_readonly_wcb_closed=1u;
        uint8_t invalid_fcb_header[512];
        for(unsigned i=0u;i<sizeof invalid_fcb_header;i++)
            invalid_fcb_header[i]=acp_sys_header[i];
        invalid_fcb_header[1]=0u;
        if(port_f11b_create_fcb(&acp_context,invalid_fcb_header,
              acp_header_lbn)!=0||
           acp_context.status!=PORT_F11_87_BADFILEHDR||
           executive_acp_allocation_source!=fcb_source||
           executive_irps.free_count!=irp_free_before||
           executive_lrps.free_count!=lrp_free_before||
           executive_srps.free_count!=srp_free_before||
           executive_np_pool.allocations!=pool_allocations_before||
           port_exe_np_pool_check(&executive_np_pool)!=PORT_EXE_NP_POOL_OK)
            return PORT_SYS_EXEC_BAD_PARAMETERS;
        p->result->dq_acp_failed_fcb_reclaimed=1u;
        p->result->dq_ddt_published=1u;
        p->result->dqa_ioc_status=PORT_V37_IOC_OK;
        p->result->dqa_ioc_ddb=found.ddb;
        p->result->dqa_ioc_ucb=found.ucb;
        p->result->dq_published_f11_read_bytes=chain_done;
        p->result->dq_published_f11_read_irps=f11ctx.reads;
        p->result->dq_acp_header_lbn=acp_header_lbn;
        p->result->dq_acp_header_reads=acp_io.disk.reads;
        p->result->dq_acp_header=acp_sys_header;
        p->result->dq_acp_file=sys_fid.number;
        p->result->dq_acp_sequence=sys_fid.sequence;
        p->result->dq_acp_volume=sys_fid.volume;
        p->result->dq_acp_fcb=acp_sys_fcb;
        p->result->dq_acp_file_blocks=acp_sys_fcb->file_blocks;
        p->result->dq_acp_alloc_source=fcb_source;
        p->result->dq_acp_np_allocations=executive_np_pool.allocations;
        p->result->signature=PORT_SYS_EXEC_DQ_DDT_PUBLISH_TAG;
        return PORT_SYS_EXEC_NORMAL;
    }
    if(p->phase==PORT_SYS_EXEC_INIT_RELOCDRV_DQ){
        /* Original RELOCDRV.MAR 103-262: interpret exact DQ DPT_STORE
         * source program on isolated copies, without a live DQ driver. */
        if(!executive_dqddt_ready||!executive_dq_fdt_ready||
           !executive_ioc_logical_ready||!executive_devicedb_ready||
           executive_relocdrv_dq_ready||
           port_v37_devices_check(executive_devicedb_bytes.data,
              sizeof executive_devicedb_bytes.data,
              (uint32_t)(uintptr_t)executive_devicedb_bytes.data,
              &executive_devicedb_result)!=PORT_V37_DEV_OK||
           port_relocdrv_dq_init(&executive_relocdrv_dq,
              executive_devicedb_bytes.data,
              sizeof executive_devicedb_bytes.data,
              &executive_devicedb_result,&executive_dqddt,
              &executive_dq_fdt)!=PORT_RELOCDRV_OK||
           port_relocdrv_dq_check(&executive_relocdrv_dq)!=
              PORT_RELOCDRV_OK||
           port_relocdrv_dq_footprint(&executive_relocdrv_dq,
              executive_devicedb_bytes.data,
              sizeof executive_devicedb_bytes.data)!=PORT_RELOCDRV_OK||
           port_relocdrv_dq_guard(executive_devicedb_bytes.data,
              sizeof executive_devicedb_bytes.data,
              &executive_devicedb_result)!=PORT_RELOCDRV_OK)
            return PORT_SYS_EXEC_BAD_PARAMETERS;
        executive_relocdrv_dq_ready=1u;
        p->result->dq_dpt_initialized=&executive_relocdrv_dq;
        p->result->dq_dpt_check=port_relocdrv_dq_check;
        p->result->signature=PORT_SYS_EXEC_RELOCDRV_DQ_TAG;
        return PORT_SYS_EXEC_NORMAL;
    }
    if(p->phase==PORT_SYS_EXEC_INIT_DDT_RELOC_PRIVATE){
        /* RELOCDRV.MAR 316-349: original IOC$RELOC_DDT walks DDT/FDT
         * bytes on genuine resident supervisor memory, never active DQA0. */
        if(!executive_relocdrv_dq_ready||!executive_dqddt_ready||
           executive_relocated_private_ready||
           executive_dqddt.driver_bound||executive_relocdrv_dq.driver_bound)
            return PORT_SYS_EXEC_BAD_PARAMETERS;
        for(unsigned i=0;i<PORT_DQDDT_IMAGE_BYTES;i++)
            executive_relocated_private.bytes[i]=executive_dqddt.image[i];
        struct port_ddtreloc_image im={executive_relocated_private.bytes,
           sizeof executive_relocated_private.bytes,
           (uint32_t)(uintptr_t)executive_relocated_private.bytes};
        if(port_ddtreloc_apply(&im)!=PORT_DDTRELOC_OK)
            return PORT_SYS_EXEC_BAD_PARAMETERS;
        executive_relocated_private_ready=1u;
        if(executive_private_reloc_check(im.image,im.length,im.pc_base)!=
           PORT_DDTRELOC_OK)return PORT_SYS_EXEC_BAD_PARAMETERS;
        if(port_dqddt_activate(&executive_dqddt,&executive_dq_fdt,
              &executive_native_dqddt,
              (uint32_t)(uintptr_t)executive_native_dqddt.image)!=
                 PORT_DQDDT_OK||
           port_dqddt_native_validate(&executive_dqddt,&executive_dq_fdt,
              &executive_native_dqddt)!=PORT_DQDDT_OK)
            return PORT_SYS_EXEC_BAD_PARAMETERS;
        p->result->dq_relocated_private=im.image;
        p->result->dq_relocated_private_base=im.pc_base;
        p->result->dq_private_relocation_check=executive_private_reloc_check;
        p->result->dq_native_ddt=&executive_native_dqddt;
        p->result->dq_native_ddt_check=port_dqddt_native_validate;
        p->result->signature=PORT_SYS_EXEC_DDT_RELOC_PRIVATE_TAG;
        return PORT_SYS_EXEC_NORMAL;
    }
    if(p->phase==PORT_SYS_EXEC_INIT_IOC_LOGICAL){
        /* IOSUBPAGD.MAR 70$-330$: SYS$DISK and SYS$SYSDEVICE traverse
         * original bootstrap LOG$TRANSLATE and physical DDB/UCB lists.
         * DQA0 is an unbound DDT; ACP, CCB and $ASSIGN cannot execute. */
        if(!executive_ioc_ready||!executive_boot_system_log_ready||
           !executive_fin_database_ready||!executive_devicedb_ready||
           executive_ioc_logical_ready||!executive_dq_bootread_ready||
           executive_devicedb_result.incomplete_driver_mask!=7u||
           port_log_system_check(&executive_boot_system_log)!=PORT_LOG_OK)
            return PORT_SYS_EXEC_BAD_PARAMETERS;
        const uint32_t base=(uint32_t)(uintptr_t)executive_devicedb_bytes.data;
        const char *const names[3]={"SYS$DISK","SYS$SYSDEVICE","NLA0:"};
        const size_t lengths[3]={8u,13u,5u};
        const uint32_t expect_status[3]={PORT_V37_IOC_UNBOUND,
            PORT_V37_IOC_UNBOUND,PORT_V37_IOC_OK};
        const uint32_t expect_ucb[3]={executive_devicedb_result.ucb[0],
            executive_devicedb_result.ucb[0],executive_devicedb_result.ucb[4]};
        for(unsigned i=0;i<3u;i++){
            if(port_devlog_search(&executive_boot_system_log,
                  executive_devicedb_bytes.data,PORT_V37_DEVICE_BYTES,base,
                  &executive_devicedb_result,names[i],lengths[i],
                  PORT_V37_IOC_MODE_PHYSICAL,&executive_device_log_samples[i])!=
                     expect_status[i]||
               executive_device_log_samples[i].device.ucb!=expect_ucb[i]||
               executive_device_log_samples[i].translations!=1u||
               executive_device_log_samples[i].concealed!=(i==1u)||
               executive_device_log_samples[i].driver_bound!=(i==2u))
                return PORT_SYS_EXEC_BAD_PARAMETERS;
        }
        if(executive_device_log_samples[0].length!=6u||
           executive_device_log_samples[1].length!=7u||
           executive_device_log_samples[0].physical[0]!='_'||
           executive_device_log_samples[0].physical[1]!='D'||
           executive_device_log_samples[0].physical[2]!='Q'||
           executive_device_log_samples[0].physical[3]!='A'||
           executive_device_log_samples[0].physical[4]!='0'||
           executive_device_log_samples[0].physical[5]!=':'||
           executive_device_log_samples[1].physical[0]!='_'||
           executive_device_log_samples[1].physical[1]!='_'||
           executive_device_log_samples[1].physical[2]!='D'||
           executive_device_log_samples[1].physical[3]!='Q'||
           executive_device_log_samples[1].physical[4]!='A'||
           executive_device_log_samples[1].physical[5]!='0'||
           executive_device_log_samples[1].physical[6]!=':'||
           executive_device_log_samples[2].length!=4u||
           !executive_device_log_samples[2].last_notran)
            return PORT_SYS_EXEC_BAD_PARAMETERS;
        struct port_devlog_trace invalid={0};
        if(port_devlog_search(&executive_boot_system_log,
               executive_devicedb_bytes.data,PORT_V37_DEVICE_BYTES,base,
               &executive_devicedb_result,"N::DQA0",7u,
               PORT_V37_IOC_MODE_PHYSICAL,&invalid)!=PORT_V37_IOC_NONLOCAL||
           invalid.length!=0u||invalid.device.ucb!=0u)
            return PORT_SYS_EXEC_BAD_PARAMETERS;
        executive_ioc_logical_ready=1u;
        p->result->ioc_logical_samples=executive_device_log_samples;
        p->result->ioc_logical_search=port_devlog_search;
        p->result->signature=PORT_SYS_EXEC_IOC_LOGICAL_TAG;
        return PORT_SYS_EXEC_NORMAL;
    }
    if(p->phase==PORT_SYS_EXEC_INIT_IOC_LOOKUP){
        /* Source IOSUBPAGD physical IOC$SEARCHDEV on actual resident
         * DEVICEDAT pointer records. The NLA software DDT is now linked,
         * but $ASSIGN, LOG$TRANSLATE and IOC mutex remain missing. */
        if(!executive_conport_ready||!executive_devicedb_ready||
           executive_ioc_ready||!executive_nl_ddt_ready ||
           executive_devicedb_result.incomplete_driver_mask!=7u)
            return PORT_SYS_EXEC_BAD_PARAMETERS;
        const uint32_t base=(uint32_t)(uintptr_t)executive_devicedb_bytes.data;
        const char *const names[3]={"__OPA0:","MBA1:","NLA0:"};
        const size_t sizes[3]={7u,5u,5u};
        const uint32_t expected[3]={executive_devicedb_result.ucb[1],
             executive_devicedb_result.ucb[2],executive_devicedb_result.ucb[4]};
        for(unsigned i=0;i<3u;i++){
            if(port_v37_ioc_searchdev(executive_devicedb_bytes.data,
                PORT_V37_DEVICE_BYTES,base,&executive_devicedb_result,
                names[i],sizes[i],&executive_ioc_samples[i])!=
                   (i==2u?PORT_V37_IOC_OK:PORT_V37_IOC_UNBOUND) ||
                executive_ioc_samples[i].ucb!=expected[i] ||
                executive_ioc_samples[i].driver_bound!=(i==2u) ||
                executive_ioc_samples[i].concealed!=(i==0u))
                return PORT_SYS_EXEC_BAD_PARAMETERS;
        }
        struct port_v37_ioc_result ignored={0};
        if(port_v37_ioc_searchdev(executive_devicedb_bytes.data,
             PORT_V37_DEVICE_BYTES,base,&executive_devicedb_result,
             "MBA3:",5u,&ignored)!=PORT_V37_IOC_NO_SUCH ||
           port_v37_ioc_searchdev(executive_devicedb_bytes.data,
             PORT_V37_DEVICE_BYTES,base,&executive_devicedb_result,
             "SYS$DISK",8u,&ignored)!=PORT_V37_IOC_LOGICAL_PENDING ||
           port_v37_ioc_searchdev(executive_devicedb_bytes.data,
             PORT_V37_DEVICE_BYTES,base,&executive_devicedb_result,
             "N::MBA1",7u,&ignored)!=PORT_V37_IOC_NONLOCAL)
            return PORT_SYS_EXEC_BAD_PARAMETERS;
        executive_ioc_ready=1u;
        p->result->ioc_samples=executive_ioc_samples;
        p->result->ioc_search=port_v37_ioc_searchdev;
        p->result->signature=PORT_SYS_EXEC_IOC_RESULT_TAG;
        return PORT_SYS_EXEC_NORMAL;
    }
    if(p->phase==PORT_SYS_EXEC_INIT_CONPORT){
        /* Original CONINTDSP port actions operate on a source-layout
         * OPA0 UCB copy; actual TT class driver/TERMINALIO is absent.
         * No device output can be claimed from this diagnostic. */
        if(!executive_devicedb_ready || executive_conport_ready ||
           executive_devicedb_result.incomplete_driver_mask!=7u)
            return PORT_SYS_EXEC_BAD_PARAMETERS;
        for(unsigned i=0;i<PORT_V37_OPA_UCB_SIZE;i++)
            executive_con_scratch.bytes[i]=
                executive_devicedb_bytes.data[PORT_V37_UCB_OPA_OFF+i];
        uint8_t *u=executive_con_scratch.bytes;
        if(port_v37_con_xoff(u,PORT_V37_OPA_UCB_SIZE,0)!=PORT_V37_CON_DEFERRED ||
           port_v37_con_xon(u,PORT_V37_OPA_UCB_SIZE,0)!=PORT_V37_CON_DEFERRED ||
           port_v37_con_stop(u,PORT_V37_OPA_UCB_SIZE)!=PORT_V37_CON_OK ||
           port_v37_con_stop2(u,PORT_V37_OPA_UCB_SIZE,7u)!=PORT_V37_CON_OK ||
           port_v37_con_get32(u,PORT_V37_CON_DUETIM)!=9u)
            return PORT_SYS_EXEC_BAD_PARAMETERS;
        port_v37_con_set16(u,PORT_V37_CON_HOLD,
            (uint16_t)(port_v37_con_get16(u,PORT_V37_CON_HOLD)|PORT_V37_CON_BURST));
        port_v37_con_set16(u,PORT_V37_CON_OUTLEN,17u);
        if(port_v37_con_resume(u,PORT_V37_OPA_UCB_SIZE,100u,0)!=PORT_V37_CON_DEFERRED ||
           port_v37_con_get32(u,PORT_V37_CON_DUETIM)!=106u ||
           port_v37_con_get32(u,PORT_V37_CON_STATE)&PORT_V37_CON_CTRLS ||
           port_v37_con_abort(u,PORT_V37_OPA_UCB_SIZE,UINT32_MAX)!=PORT_V37_CON_OK ||
           port_v37_con_get32(u,PORT_V37_CON_DUETIM)!=1u ||
           port_v37_con_get16(u,PORT_V37_CON_HOLD)!=
               (PORT_V37_CON_XOFF|PORT_V37_CON_XON) ||
           port_v37_con_get16(u,PORT_V37_CON_STS)!=
               (PORT_V37_UCB_ONLINE|PORT_V37_CON_TIM))
            return PORT_SYS_EXEC_BAD_PARAMETERS;
        executive_conport_ready=1u;
        p->result->console_action_scratch=u;
        p->result->console_action_result=PORT_V37_CON_DEFERRED;
        p->result->signature=PORT_SYS_EXEC_CON_RESULT_TAG;
        return PORT_SYS_EXEC_NORMAL;
    }
    if(p->phase==PORT_SYS_EXEC_INIT_DEVICE_DB){
        /* DEVICEDAT.MAR IOC$GL_DEVLIST: copy real pointer-linked
         * BOOT->OPA->MBA->NLA DDB/UCB templates into resident SYS.EXE.
         * Only NLDRIVER has an implemented native DDTAB/FUNCTAB image.
         * Other three driver DDTs and original MNTVER remain unported. */
        if(!executive_bootdev_scan_ready || !executive_dpt_stages_ready ||
           !executive_syslog_ready || executive_devicedb_ready ||
           (uintptr_t)executive_devicedb_bytes.data>UINT32_MAX ||
           port_v37_devices_prelink(executive_devicedb_bytes.data,
               sizeof executive_devicedb_bytes.data,
               (uint32_t)(uintptr_t)executive_devicedb_bytes.data,
               &executive_devicedb_result)!=PORT_V37_DEV_OK ||
           (uintptr_t)executive_nl_ddt.bytes>UINT32_MAX ||
           (uintptr_t)port_v37_nl_ioc_return>UINT32_MAX ||
           (uintptr_t)port_v37_nl_fdt_read>UINT32_MAX ||
           (uintptr_t)port_v37_nl_fdt_write>UINT32_MAX ||
           port_v37_nl_ddt_build(executive_nl_ddt.bytes,
               sizeof executive_nl_ddt.bytes,
               (uint32_t)(uintptr_t)executive_nl_ddt.bytes,
               (uint32_t)(uintptr_t)port_v37_nl_ioc_return,
               (uint32_t)(uintptr_t)port_v37_nl_fdt_read,
               (uint32_t)(uintptr_t)port_v37_nl_fdt_write)!=PORT_V37_NL_DDT_OK ||
           port_v37_nl_ddt_check(executive_nl_ddt.bytes,
               sizeof executive_nl_ddt.bytes,
               (uint32_t)(uintptr_t)executive_nl_ddt.bytes,
               (uint32_t)(uintptr_t)port_v37_nl_ioc_return,
               (uint32_t)(uintptr_t)port_v37_nl_fdt_read,
               (uint32_t)(uintptr_t)port_v37_nl_fdt_write)!=PORT_V37_NL_DDT_OK ||
           port_v37_nl_ddt_bind(executive_devicedb_bytes.data,
               sizeof executive_devicedb_bytes.data,
               (uint32_t)(uintptr_t)executive_devicedb_bytes.data,
               &executive_devicedb_result,
               (uint32_t)(uintptr_t)executive_nl_ddt.bytes)!=PORT_V37_NL_DDT_OK ||
           port_v37_devices_check(executive_devicedb_bytes.data,
               sizeof executive_devicedb_bytes.data,
               (uint32_t)(uintptr_t)executive_devicedb_bytes.data,
               &executive_devicedb_result)!=PORT_V37_DEV_OK)
            return PORT_SYS_EXEC_BAD_PARAMETERS;
        executive_nl_ddt_ready=1u;
        executive_devicedb_ready=1u;
        p->result->device_db=&executive_devicedb_result;
        p->result->device_db_bytes=executive_devicedb_bytes.data;
        p->result->null_ddt_bytes=executive_nl_ddt.bytes;
        p->result->null_ddt_base=(uint32_t)(uintptr_t)executive_nl_ddt.bytes;
        p->result->device_db_check=port_v37_devices_check;
        p->result->signature=PORT_SYS_EXEC_DEVICE_RESULT_TAG;
        return PORT_SYS_EXEC_NORMAL;
    }
    if(p->phase==PORT_SYS_EXEC_INIT_BOOTDEV_SCAN){
        /* INIT.MAR INI_BOOTDEVIC adapter traversal. Its input is the
         * genuine INILOA registered I/O-port table from this boot.
         * No DPT, DDB, UCB or VAX device is synthesized by this scan. */
        if(!executive_syslog_ready || !executive_null_fdt_ready ||
           executive_bootdev_scan_ready || !p->bootdev_state ||
           port_ini_bootdev_scan(p->bootdev_state,
                    &executive_bootdev_scan)!=PORT_INI_BOOTDEV_OK)
            return PORT_SYS_EXEC_BAD_PARAMETERS;
        executive_bootdev_scan_ready=1u;
        p->result->bootdev_scan=&executive_bootdev_scan;
        p->result->signature=PORT_SYS_EXEC_BOOTDEV_RESULT_TAG;
        return PORT_SYS_EXEC_NORMAL;
    }
    if(p->phase==PORT_SYS_EXEC_LINK_SYSLOA){
        /* This is INIT.MAR LINK_VEC/EXE$AL_LOAVEC, in resident SYS.EXE. */
        if(!p->sysloa_descriptor || !p->sysloa_bytes ||
           !port_sysloavec_link(&executive_sysloa,p->sysloa_descriptor,
                p->sysloa_bytes,PORT_SYSLOA386_DYN_LOADCODE))
            return PORT_SYS_EXEC_BAD_PARAMETERS;
        if(!port_sysloavec_address(&executive_sysloa,PORT_SYSLOA_INIBOOTADP)||
           !port_sysloavec_address(&executive_sysloa,PORT_SYSLOA_INIPROCREG))
            return PORT_SYS_EXEC_BAD_PARAMETERS;
        p->result->sysloa_linked=1u;
        p->result->signature=PORT_SYS_EXEC_LINK_RESULT_TAG;
        return PORT_SYS_EXEC_NORMAL;
    }
    if(p->phase!=PORT_SYS_EXEC_INIT_RESIDENT ||
       p->initialized_images!=3u || !executive_sysloa.loaded ||
       !executive_lockdb_ready || !executive_procvec_ready ||
       !executive_phvswap_ready || !executive_mpw_ready ||
       !executive_pflvec_ready || !executive_interrupt_stack_ready ||
       !executive_system_wcb_ready || !executive_dpt_stages_ready ||
       !executive_null_fdt_ready || !executive_bootdev_scan_ready ||
       !executive_ioc_ready || !executive_post_ready || !p->cpu_ops || !p->boot_ops)return PORT_SYS_EXEC_BAD_PARAMETERS;

    /* Hardware-dependent EXE$INIBOOTADP/EXE$INIPROCREG are called through
     * the self-relative vectors that were linked into SYS.EXE. */
    uint32_t (*boot)(const struct port_sysloa386_boot_ops *,
                     struct port_sysloa386_boot_result *)=
       (void *)(uintptr_t)port_sysloavec_address(&executive_sysloa,
                                                     PORT_SYSLOA_INIBOOTADP);
    uint32_t (*cpu)(const struct port_sysloa386_cpu_ops *,
                    struct port_sysloa386_cpu_result *)=
       (void *)(uintptr_t)port_sysloavec_address(&executive_sysloa,
                                                     PORT_SYSLOA_INIPROCREG);
    struct port_sysloa386_boot_result boot_info={0};
    struct port_sysloa386_cpu_result cpu_info={0};
    if(!boot || !cpu ||
       boot(p->boot_ops,&boot_info)!=PORT_SYSLOA386_OK ||
       cpu(p->cpu_ops,&cpu_info)!=PORT_SYSLOA386_OK ||
       boot_info.home_block_lbn!=1u || !boot_info.cluster_factor)
        return PORT_SYS_EXEC_BAD_PARAMETERS;

    /* INIT's SCB, scheduler queue and clock initialization is partial;
     * no process activation, LOGINOUT or DCL can be inferred from it. */
    port_exe_sched_init(&executive_scheduler,p->pcb_type);
    port_exe_scb_init(executive_scb);
    port_exe_systime_store(&executive_clock,p->system_time);
    const struct port_exe_scb_slot *acv=port_exe_scb_get(executive_scb,0x20u);
    const struct port_exe_scb_slot *pf=port_exe_scb_get(executive_scb,0x24u);
    if(!acv || !pf || acv->pc_idt_vector!=13u || pf->pc_idt_vector!=14u)
        return PORT_SYS_EXEC_BAD_PARAMETERS;
    p->result->scheduler_ready_mask=executive_scheduler.ready_mask;
    p->result->scheduler_pcb_type=executive_scheduler.pcb_type;
    p->result->scb_access_violation_pc_vector=acv->pc_idt_vector;
    p->result->scb_page_fault_pc_vector=pf->pc_idt_vector;
    p->result->system_time_low=executive_clock.low;
    p->result->system_time_high=executive_clock.high;
    p->result->resident_clock=&executive_clock;
    p->result->pit_tick=executive_pit_clock_tick;
    p->result->sysloa_linked=1u;
    p->result->sysloa_idtr_base=cpu_info.idt_base;
    p->result->sysloa_irq_ticks=cpu_info.irq_ticks;
    p->result->sysloa_home_lbn=boot_info.home_block_lbn;
    p->result->sysloa_cr0=cpu_info.cr0;
    p->result->signature=PORT_SYS_EXEC_RESULT_TAG;
    return PORT_SYS_EXEC_NORMAL;
}
