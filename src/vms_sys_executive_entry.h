#ifndef VMS_SYS_EXECUTIVE_ENTRY_H
#define VMS_SYS_EXECUTIVE_ENTRY_H
#include <stdint.h>
#include "vms_sysloa386.h"
#include "vms_exe_gettim.h"
#include "vms_sysboot_pktmem.h"
#include "vms_ini_packet_queues.h"
#include "vms_ini_lockdb.h"
#include "vms_ini_procvec.h"
#include "vms_ini_phv_swap.h"
#include "vms_ini_mpw.h"
#include "vms_ini_pflvec.h"
#include "vms_exe_sched.h"
#include "vms_ini_syswcb.h"
#include "vms_ini_syslog.h"
#include "vms_ini_dpt_templates.h"
#include "vms_ini_bootdev_scan.h"
#include "vms_dq_fdt.h"
#include "vms_dqddt.h"
#include "vms_dq_crb.h"
#include "vms_relocdrv.h"
#include "vms_dq_bootread.h"
#include "vms_dq_index_header.h"
#include "vms_dq_mfd_header.h"
#include "vms_dq_dir.h"
#include "vms_dq_imageprobe.h"
#include "vms_ini_fin_database.h"
#include "vms_log_system.h"
#include "vms_devlog_search.h"
#include "vms_ini_devicedb.h"
#include "vms_con_port.h"
#include "vms_ioc_phys.h"
#include "vms_qio_post.h"
#include "vms_nl_ddt.h"
#include "vms_iopost_end.h"
#include "vms_ddtreloc.h"
#include "vms_sys85.h"
#include "vms_dqddt_ioc_bind.h"
/* The resident SYS.EXE entry carries INIT.MAR's CPU-code vector binding
 * and subsequent source-ordered initialization as separate phases. */
enum { PORT_SYS_EXEC_SIGNATURE=0x37334d56u,PORT_SYS_EXEC_VERSION=32u,
       PORT_SYS_EXEC_NORMAL=1u,PORT_SYS_EXEC_BAD_PARAMETERS=2u,
       PORT_SYS_EXEC_RESULT_TAG=0x58455359u,PORT_SYS_EXEC_PCB_KIND=1u,
       PORT_SYS_EXEC_LINK_SYSLOA=1u,PORT_SYS_EXEC_INIT_RESIDENT=2u,
       PORT_SYS_EXEC_LINK_RESULT_TAG=0x4b4e4c53u,
       PORT_SYS_EXEC_INIT_IOQUEUES=3u,PORT_SYS_EXEC_PACKET_RESULT_TAG=0x51504b54u,
       PORT_SYS_EXEC_INIT_LOCKDB=4u, PORT_SYS_EXEC_LOCK_RESULT_TAG=0x4b434f4cu,
       PORT_SYS_EXEC_INIT_PROCVEC=5u, PORT_SYS_EXEC_PV_RESULT_TAG=0x56424350u,
       PORT_SYS_EXEC_INIT_PHVSWAP=6u, PORT_SYS_EXEC_PHV_RESULT_TAG=0x56574850u,
       PORT_SYS_EXEC_INIT_MPW=7u, PORT_SYS_EXEC_MPW_RESULT_TAG=0x57504d50u,
       PORT_SYS_EXEC_INIT_INTSTKLM=8u,PORT_SYS_EXEC_INTSTK_RESULT_TAG=0x4b54534cu,
       PORT_SYS_EXEC_INIT_PFLVEC=9u,PORT_SYS_EXEC_PFL_RESULT_TAG=0x564c4650u,
       PORT_SYS_EXEC_INIT_SYSWCB=10u,PORT_SYS_EXEC_SYSWCB_RESULT_TAG=0x42435753u,
       PORT_SYS_EXEC_INIT_SYSLOG=11u,PORT_SYS_EXEC_SYSLOG_RESULT_TAG=0x474f4c53u,
       PORT_SYS_EXEC_INIT_NL_FDT=12u, PORT_SYS_EXEC_NL_RESULT_TAG=0x4c554e4cu,
       PORT_SYS_EXEC_INIT_BOOTDEV_SCAN=13u, PORT_SYS_EXEC_BOOTDEV_RESULT_TAG=0x56444249u,
       PORT_SYS_EXEC_INIT_DPT_STAGES=14u, PORT_SYS_EXEC_DPT_RESULT_TAG=0x54504453u,
       PORT_SYS_EXEC_INIT_DEVICE_DB=15u,PORT_SYS_EXEC_DEVICE_RESULT_TAG=0x42444456u,
       PORT_SYS_EXEC_INIT_CONPORT=16u,PORT_SYS_EXEC_CON_RESULT_TAG=0x4e4f4350u,
       PORT_SYS_EXEC_INIT_IOC_LOOKUP=17u,PORT_SYS_EXEC_IOC_RESULT_TAG=0x434f4950u,
       PORT_SYS_EXEC_INIT_QIO_POST=18u,PORT_SYS_EXEC_POST_RESULT_TAG=0x54534f50u,
       PORT_SYS_EXEC_INIT_IOPOST_END=19u,PORT_SYS_EXEC_IOPOST_END_TAG=0x444e4549u,
       PORT_SYS_EXEC_VERIFY_IOPOST_INT=20u,
       PORT_SYS_EXEC_IOPOST_BIND_TAG=0x42494950u,
       PORT_SYS_EXEC_INIT_DQ_FDT=21u, PORT_SYS_EXEC_DQ_RESULT_TAG=0x54465144u,
       PORT_SYS_EXEC_INIT_DQ_BOOTREAD=22u, PORT_SYS_EXEC_DQ_BOOTREAD_TAG=0x44425144u,
       PORT_SYS_EXEC_INIT_FIN_DATABASE=23u,PORT_SYS_EXEC_FIN_DATABASE_TAG=0x42444e46u,
       PORT_SYS_EXEC_INIT_SYSTEM_LOG=24u,PORT_SYS_EXEC_SYSTEM_LOG_TAG=0x534c474cu,
       PORT_SYS_EXEC_INIT_IOC_LOGICAL=25u,PORT_SYS_EXEC_IOC_LOGICAL_TAG=0x4c434f49u,
       PORT_SYS_EXEC_INIT_DQ_DDT_STAGE=26u,PORT_SYS_EXEC_DQ_DDT_TAG=0x54444451u,
       PORT_SYS_EXEC_INIT_RELOCDRV_DQ=27u,PORT_SYS_EXEC_RELOCDRV_DQ_TAG=0x44524c52u,
       PORT_SYS_EXEC_INIT_DDT_RELOC_PRIVATE=28u,
       PORT_SYS_EXEC_DDT_RELOC_PRIVATE_TAG=0x54524c44u,
       PORT_SYS_EXEC_INIT_DQ_CRB_READ=29u,PORT_SYS_EXEC_DQ_CRB_READ_TAG=0x42524344u,
       PORT_SYS_EXEC_INIT_DQ_DDT_PUBLISH=30u,PORT_SYS_EXEC_DQ_DDT_PUBLISH_TAG=0x42515044u };
/* This is an explicitly privileged early-boot disk path. Hardware is real
 * ATA PIO; the original DQ disk DDT and Files-11 ACP are NOT bound. */
struct port_sys_exec_dq_boot_settings {
    port_dq_native_read_fn read_sector;
    void *context;
    uint16_t boot_unit,reserved;
    port_dq_crb_handler irq_handler;
};
/* The IA-32 privilege-transition stack is the first live counterpart of
 * INIT.MAR EXE$GL_INTSTK; source VAX IPL stack switching is not present. */
/* SYSBOOT passes the genuine Files-11 SYS.EXE extent map through the
 * source BOO$L_SYS_MAP interface.  The native header below is explicitly
 * an incomplete WCB until original DEC layout constants are established. */
struct port_sys_exec_wcb_settings {
    uint8_t *normalized_boot_map;
    uint32_t boot_map_capacity;
    uint32_t expected_extent_count;
    uint32_t expected_first_lbn;
    uint32_t expected_total_blocks;
};
struct port_sys_exec_intstack_settings {
    uint32_t stack_high,stack_pages,lock_extra_bytes;
};
struct port_sys_exec_result {
    uint32_t signature;
    uint32_t scheduler_ready_mask;
    uint32_t scheduler_pcb_type;
    uint32_t scb_access_violation_pc_vector;
    uint32_t scb_page_fault_pc_vector;
    uint32_t system_time_low;
    uint32_t system_time_high;
    uint32_t sysloa_linked;
    uint32_t sysloa_idtr_base;
    uint32_t sysloa_irq_ticks;
    uint32_t sysloa_home_lbn;
    uint32_t sysloa_cr0;
    const struct port_exe_systime *resident_clock;
    void (*pit_tick)(void);
    const struct port_ini_packet_list *irp_queue,*lrp_queue,*srp_queue;
    uint32_t irp_count,lrp_count,srp_count;
    const struct port_ini_lockdb *lock_database;
    uint32_t lock_max_id,lock_hash_slots,lock_process_bitmap_bytes;
    const struct port_ini_phvswap *process_header_vector;
    const struct port_ini_mpw *modified_page_writer_map;
    const struct port_ini_interrupt_stack *interrupt_stack;
    const struct port_ini_pflvec *pagefile_vector;
    const struct port_ini_syswcb *system_wcb;
    const struct port_ini_syslog_result *syslog;
    /* Original MB/NL/OP 56-byte DPT prologues only: not linked to IOC. */
    const uint8_t (*dpt_stages)[PORT_V37_DPT_STAGE_BYTES];
    /* Guest-resident FDT exists; there is no IOC$GL_DPTLIST attachment yet. */
    const struct port_sys85_null_dpt *null_driver_dpt;
    uint32_t (*null_driver_io)(uint16_t,void *,port_sys85_finish_io_fn);
    /* Source-first adapter selection only; DDB/UCB allocation is pending. */
    const struct port_ini_bootdev_result *bootdev_scan;
    /* DQDRIVER source FDT is *staged* only; DQ_DRIVER/ACP unlinked. */
    const struct port_dq_fdt *dq_source_fdt;
    uint32_t (*dq_fdt_classify)(const struct port_dq_fdt *,size_t,uint16_t,
                                struct port_dq_fdt_decision *);
    /* Resident one-sector physical LBN1 boot read, no public QIO. */
    const struct port_dq_bootread_result *dq_bootread;
    const uint16_t *dq_bootread_sector;
    /* Second real privileged READPBLK, LBN0. Not a public disk channel. */
    const struct port_dq_bootread_result *dq_bootzero;
    const uint16_t *dq_bootzero_sector;
    /* Third real physical READPBLK verifies INDEXF.SYS' FH2 in resident RAM.
     * This is still pre-ACP, with the DQ DDT unbound. */
    const struct port_dq_index_result *dq_index_header;
    const uint16_t *dq_index_sector;
    /* Fourth native READPBLK resolves and validates MFD FID 4,4,0. */
    const struct port_dq_mfd_result *dq_mfd_header;
    const uint16_t *dq_mfd_sector;
    /* Resident FIL$FINDFILID returns these real directory FIDs. */
    const struct port_dq_dir_fid *dq_sysexe_dir_fid,*dq_sysexecutable_fid;
    uint32_t dq_dir_irp_last_count;
    /* Ninth resident physical IRP validates the real native SYS.EXE ELF
     * file block without activating it as a user process. */
    const struct port_dq_image_result *dq_image_probe;
    /* Original INIT.MAR FIN_DATABASE storage stage; no LOG$TRANSLATE. */
    const struct port_ini_fin_result *boot_device_names;
    /* Original LOGNAMSUB.MAR limited bootstrap SYSTEM names only. */
    const struct port_log_system_table *boot_system_log_table;
    uint32_t (*boot_system_translate)(const struct port_log_system_table *,
        const char *,size_t,uint32_t,uint8_t *,size_t,struct port_log_result *);
    /* Source IOC$SEARCHDEV resolves boot SYSTEM logical aliases to prelinked
     * physical DDB/UCB only; original DQ disk DDT/ACP stay unbound. */
    const struct port_devlog_trace *ioc_logical_samples;
    uint32_t (*ioc_logical_search)(const struct port_log_system_table *,
        const uint8_t *,size_t,uint32_t,const struct port_v37_device_result *,
        const char *,size_t,unsigned,struct port_devlog_trace *);
    /* Source DQDRIVER DPTAB/DDTAB and exact 136-byte FUNCTAB staged in
     * resident SYS.EXE. Private physical READ/WRITE actions only; DQA0 DDT=0. */
    const struct port_dqddt_stage *dq_ddt_stage;
    uint32_t (*dq_ddt_function)(const struct port_dqddt_stage *,
        const struct port_dq_fdt *,uint16_t);
    uint32_t (*dq_ddt_dispatch_bootwrite)(const struct port_dqddt_stage *,
        const struct port_dq_fdt *,const struct port_dq_bootread_request *,
        struct port_dq_bootread_result *,uint32_t);
    uint32_t (*dq_ddt_startio)(const struct port_dqddt_stage *,
        const struct port_dqddt_native *,const struct port_dq_fdt *,
        const struct port_dq_bootread_request *,struct port_dq_bootread_result *);
    const struct port_dq_crb *dq_crb;
    /* Private stage-validated READPBLK bridge result, not DDB/CRB binding. */
    const struct port_dq_bootread_result *dq_ddt_bootread;
    const uint16_t *dq_ddt_bootread_sector;
    /* Original RELOCDRV IOC$INITDRV applies exact source DPT_STORE to
     * isolated DQA0 DDB/UCB copies; no driver/ACP/CRB publication. */
    const struct port_relocdrv_dq *dq_dpt_initialized;
    uint32_t (*dq_dpt_check)(const struct port_relocdrv_dq *);
    /* IOC$RELOC_DDT executes inside resident SYS.EXE on a private copy.
     * No action slot is callable and no DDB DDT pointer is attached. */
    const uint8_t *dq_relocated_private;
    uint32_t dq_relocated_private_base;
    uint32_t (*dq_private_relocation_check)(const uint8_t *,size_t,uint32_t);
    /* Native-pointer DDT materialized from source and published in DQA0
     * DDB/UCB. Supported actions are still private physical block I/O only. */
    const struct port_dqddt_native *dq_native_ddt;
    uint32_t (*dq_native_ddt_check)(const struct port_dqddt_stage *,
        const struct port_dq_fdt *,const struct port_dqddt_native *);
    uint32_t dq_ddt_published,dqa_ioc_status,dqa_ioc_ddb,dqa_ioc_ucb;
    uint32_t dq_published_f11_read_bytes,dq_published_f11_read_irps;
    /* DEVICEDAT DDB/UCB pointer graph; DQA0 is partially DDT/CRB bound. */
    const struct port_v37_device_result *device_db;
    const uint8_t *device_db_bytes;
    const uint8_t *null_ddt_bytes; /* native source DDTAB/FUNCTAB, 80 bytes */
    uint32_t null_ddt_base; /* real resident SYS.EXE table address */
    uint32_t (*device_db_check)(const uint8_t *,size_t,uint32_t,
         const struct port_v37_device_result *);
    /* Diagnostic source transitions only, not an active OP driver. */
    const uint8_t *console_action_scratch;
    uint32_t console_action_result;
    /* IOSUBPAGD source physical lookup on prelinked records, diagnostic.
     * Even a matched UCB has no live DDT; lookup status is UNBOUND. */
    const struct port_v37_ioc_result *ioc_samples;
    uint32_t (*ioc_search)(const uint8_t *,size_t,uint32_t,
       const struct port_v37_device_result *,const char *,size_t,
       struct port_v37_ioc_result *);
    /* Source NLDRIVER -> SYSQIOREQ EXE$FINISHIOC completed IRP -> IOC post
     * queue; IRQ dispatch, CCB, IOSB and AST delivery remain unavailable. */
    const struct port_v37_iopost_queue *iopost;
    const uint8_t *post_irp_bytes;
    uint32_t iopost_softint_count,iopost_softint_level,iopost_ucb_ops;
    uint32_t (*nl_post)(struct port_v37_qio_request *);
    /* Original IOCIOPOST.MAR kernel end-action path via a real IA-32
     * DPL0 software INT 0x90; no process IOSB, event flags or ASTs. */
    const struct port_v37_iopost_endlog *iopost_endlog;
    uint32_t (*iopost_isr)(void);
    uint32_t iopost_remaining;
    uint32_t logical_table[2];
    uint8_t lock_max_resource_depth;
    const struct port_ini_procvec *process_vector;
    const struct port_exe_sched_process *system_pcb,*null_pcb;
};
struct port_sys_exec_start {
    uint32_t signature,version;
    uint64_t system_time;
    uint8_t pcb_type;
    uint8_t initialized_images;
    uint16_t reserved;
    struct port_sys_exec_result *result;
    uint32_t phase;
    const void *sysloa_descriptor;
    uint32_t sysloa_bytes;
    const struct port_sysloa386_cpu_ops *cpu_ops;
    const struct port_sysloa386_boot_ops *boot_ops;
    const struct port_boo_packet_settings *packet_settings;
    const struct port_ini_lock_settings *lock_settings;
    const struct port_ini_proc_settings *proc_settings;
    const struct port_ini_phvswap_settings *phv_settings;
    const struct port_ini_mpw_settings *mpw_settings;
    const struct port_sys_exec_intstack_settings *intstack_settings;
    const struct port_ini_pflvec_settings *pfl_settings;
    const struct port_sys_exec_wcb_settings *syswcb_settings;
    const struct port_ini_io_state *bootdev_state;
    const struct port_sys_exec_dq_boot_settings *dq_boot_io;
};
uint32_t port_sys_executive_entry(const struct port_sys_exec_start *parameters);
#endif
