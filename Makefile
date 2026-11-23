AS=as
CC=gcc
LD=ld
OBJCOPY=objcopy
CFLAGS=-m32 -std=c11 -Os -ffreestanding -fno-pic -fno-pie -fno-stack-protector -fno-builtin -fno-plt -fno-asynchronous-unwind-tables -fno-unwind-tables -Wall -Wextra -Werror
EXEFLAGS=$(filter-out -fno-plt,$(CFLAGS)) -fplt
.PHONY: all clean test smoke
all: vms37lab.ima portdisk.img build/executive-partial.o build/boot-utilities-partial.o build/login-utilities-partial.o build/rtl-library-partial.o build/files11-acp-partial.o
build:
	mkdir -p build
build/boot.o: src/boot.s | build
	$(AS) --32 $< -o $@
build/boot.bin: build/boot.o
	$(LD) -m elf_i386 -Ttext 0x7c00 -e _start --oformat binary $< -o $@
build/start.o: src/start.s | build
	$(AS) --32 $< -o $@
build/arch.o: src/arch.c src/vms_dq_crb.h src/vms_exe_gettim.h src/vms_exe_sched.h src/vms_logintime.h src/vms_postef.h src/vms_mutex.h src/vms_condition.h src/vms_ast.h src/vms_pmap.h src/vms_pcb.h src/vms_phys.h src/vms_vmem.h src/vms_event.h src/vms_ata.h src/vms_ne2k.h src/vms_decnet.h src/vms_nsp.h src/vms_nsp_link.h src/vms_session.h src/vms_nsp_data.h src/vms_nsp_reliable.h src/vms_mirror.h src/vms_mirror_server.h src/vms_serial.h src/vms_qio.h src/vms_channels.h src/vms_userptr.h src/vms_userio.h src/vms_channels.h src/vms_kstack.h src/vms_ramcheck.h | build
	$(CC) $(CFLAGS) -c $< -o $@
build/vms_dq_crb.o: src/vms_dq_crb.c src/vms_dq_crb.h src/vms_v37_dyn_tags.h | build
	$(CC) $(CFLAGS) -c $< -o $@
build/arch_asm.o: src/arch.s | build
	$(AS) --32 $< -o $@
build/vms_sysinit_names.o: src/vms_sysinit_names.c src/vms_sysinit_names.h | build
	$(CC) $(CFLAGS) -c $< -o $@
build/vms_init_mfd.o: src/vms_init_mfd.c src/vms_init_mfd.h | build
	$(CC) $(CFLAGS) -c $< -o $@
build/vms_f11_home.o: src/vms_f11_home.c src/vms_f11_home.h | build
	$(CC) $(CFLAGS) -c $< -o $@
build/vms_filerwio.o: src/vms_filerwio.c src/vms_filerwio.h | build
	$(CC) $(CFLAGS) -c $< -o $@
build/vms_bootio.o: src/vms_bootio.c src/vms_bootio.h | build
	$(CC) $(CFLAGS) -c $< -o $@
build/vms_boot_files11.o: src/vms_boot_files11.c src/vms_boot_files11.h | build
	$(CC) $(CFLAGS) -c $< -o $@
build/vms_sysboot_map.o: src/vms_sysboot_map.c src/vms_sysboot_map.h src/vms_boot_files11.h | build
	$(CC) $(CFLAGS) -c $< -o $@
build/vms_sysboot_read.o: src/vms_sysboot_read.c src/vms_sysboot_read.h src/vms_sysboot_map.h src/vms_boot_files11.h src/vms_bootio.h | build
	$(CC) $(CFLAGS) -c $< -o $@
build/vms_initvec.o: src/vms_initvec.c src/vms_initvec.h | build
	$(CC) $(CFLAGS) -c $< -o $@
build/vms_login_interact.o: src/vms_login_interact.c src/vms_login_interact.h src/vms_login_fileio.h src/vms_logincmd.h src/vms_validate.h | build
	$(CC) $(CFLAGS) -c $< -o $@
build/vms_logincmd.o: src/vms_logincmd.c src/vms_logincmd.h | build
	$(CC) $(CFLAGS) -c $< -o $@
build/vms_login_fileio.o: src/vms_login_fileio.c src/vms_login_fileio.h | build
	$(CC) $(CFLAGS) -c $< -o $@
build/vms_vmb_rpb.o: src/vms_vmb_rpb.c src/vms_vmb_rpb.h src/vms_vmbargs.h | build
	$(CC) $(CFLAGS) -c $< -o $@
build/vms_sysmount.o: src/vms_sysmount.c src/vms_sysmount.h | build
	$(CC) $(CFLAGS) -c $< -o $@
build/vms_vmbargs.o: src/vms_vmbargs.c src/vms_vmbargs.h src/vms_bootio.h | build
	$(CC) $(CFLAGS) -c $< -o $@
build/vms_booparam.o: src/vms_booparam.c src/vms_booparam.h | build
	$(CC) $(CFLAGS) -c $< -o $@
build/vms_boot_readprompt.o: src/vms_boot_readprompt.c src/vms_boot_readprompt.h | build
	$(CC) $(CFLAGS) -c $< -o $@
build/vms_login_uart.o: src/vms_login_uart.c src/vms_login_uart.h src/vms_serial.h | build
	$(CC) $(CFLAGS) -c $< -o $@
build/vms_dcl_descr.o: src/vms_dcl_descr.c src/vms_dcl_descr.h | build
	$(CC) $(CFLAGS) -c $< -o $@
build/vms_dcl_goto.o: src/vms_dcl_goto.c src/vms_dcl_goto.h | build
	$(CC) $(CFLAGS) -c $< -o $@
build/vms_dcl_initial.o: src/vms_dcl_initial.c src/vms_dcl_initial.h | build
	$(CC) $(CFLAGS) -c $< -o $@
build/vms_dcl_command.o: src/vms_dcl_command.c src/vms_dcl_command.h | build
	$(CC) $(CFLAGS) -c $< -o $@
build/vms_dcl_readrec.o: src/vms_dcl_readrec.c src/vms_dcl_readrec.h | build
	$(CC) $(CFLAGS) -c $< -o $@
build/vms_dcl_on.o: src/vms_dcl_on.c src/vms_dcl_on.h | build
	$(CC) $(CFLAGS) -c $< -o $@
build/vms_dcl_intimages.o: src/vms_dcl_intimages.c src/vms_dcl_intimages.h | build
	$(CC) $(CFLAGS) -c $< -o $@
build/vms_dcl_inquire.o: src/vms_dcl_inquire.c src/vms_dcl_inquire.h | build
	$(CC) $(CFLAGS) -c $< -o $@
build/vms_dcl_if.o: src/vms_dcl_if.c src/vms_dcl_if.h | build
	$(CC) $(CFLAGS) -c $< -o $@
build/vms_dcl_status.o: src/vms_dcl_status.c src/vms_dcl_status.h | build
	$(CC) $(CFLAGS) -c $< -o $@
build/vms_dcl_exit.o: src/vms_dcl_exit.c src/vms_dcl_exit.h | build
	$(CC) $(CFLAGS) -c $< -o $@
build/vms_hpwd.o: src/vms_hpwd.c src/vms_hpwd.h | build
	$(CC) $(CFLAGS) -c $< -o $@
build/vms_logintime.o: src/vms_logintime.c src/vms_logintime.h | build
	$(CC) $(CFLAGS) -c $< -o $@
build/vms_rtc_x86.o: src/vms_rtc_x86.c src/vms_logintime.h | build
	$(CC) $(CFLAGS) -c $< -o $@
build/vms_uaf_v37.o: src/vms_uaf_v37.c src/vms_uaf_v37.h src/vms_validate.h | build
	$(CC) $(CFLAGS) -c $< -o $@
build/vms_validate.o: src/vms_validate.c src/vms_validate.h src/vms_uaf_v37.h src/vms_hpwd.h | build
	$(CC) $(CFLAGS) -c $< -o $@
build/vms_hpwd_selftest.o: src/vms_hpwd_selftest.c src/vms_hpwd.h | build
	$(CC) $(CFLAGS) -c $< -o $@
build/vms_mutex.o: src/vms_mutex.c src/vms_mutex.h src/vms_pcb.h | build
	$(CC) $(CFLAGS) -c $< -o $@
build/vms_mutex_selftest.o: src/vms_mutex_selftest.c src/vms_mutex.h | build
	$(CC) $(CFLAGS) -c $< -o $@
build/vms_postef.o: src/vms_postef.c src/vms_postef.h src/vms_pcb.h src/vms_event.h | build
	$(CC) $(CFLAGS) -c $< -o $@
build/vms_postef_selftest.o: src/vms_postef_selftest.c src/vms_postef.h | build
	$(CC) $(CFLAGS) -c $< -o $@
build/vms_astdel.o: src/vms_astdel.c src/vms_astdel.h src/vms_ast.h | build
	$(CC) $(CFLAGS) -c $< -o $@
build/vms_astdel_selftest.o: src/vms_astdel_selftest.c src/vms_astdel.h | build
	$(CC) $(CFLAGS) -c $< -o $@
build/vms_ast.o: src/vms_ast.c src/vms_ast.h | build
	$(CC) $(CFLAGS) -c $< -o $@
build/vms_condition.o: src/vms_condition.c src/vms_condition.h | build
	$(CC) $(CFLAGS) -c $< -o $@
build/vms_pmap.o: src/vms_pmap.c src/vms_pmap.h src/vms_pcb.h src/vms_phys.h | build
	$(CC) $(CFLAGS) -c $< -o $@
build/vms_pcb.o: src/vms_pcb.c src/vms_pcb.h src/vms_ast.h | build
	$(CC) $(CFLAGS) -c $< -o $@
build/vms_kstack.o: src/vms_kstack.c src/vms_kstack.h | build
	$(CC) $(CFLAGS) -c $< -o $@
build/vms_ne2k.o: src/vms_ne2k.c src/vms_ne2k.h | build
	$(CC) $(CFLAGS) -c $< -o $@
build/vms_decnet.o: src/vms_decnet.c src/vms_decnet.h | build
	$(CC) $(CFLAGS) -c $< -o $@
build/vms_rem_protocol.o: src/vms_rem_protocol.c src/vms_rem_protocol.h | build
	$(CC) $(CFLAGS) -c $< -o $@
build/vms_rem_protocol_selftest.o: src/vms_rem_protocol_selftest.c src/vms_rem_protocol.h | build
	$(CC) $(CFLAGS) -c $< -o $@
build/vms_nsp.o: src/vms_nsp.c src/vms_nsp.h | build
	$(CC) $(CFLAGS) -c $< -o $@
build/vms_nsp_link.o: src/vms_nsp_link.c src/vms_nsp_link.h src/vms_nsp.h | build
	$(CC) $(CFLAGS) -c $< -o $@
build/vms_nsp_data.o: src/vms_nsp_data.c src/vms_nsp_data.h | build
	$(CC) $(CFLAGS) -c $< -o $@
build/vms_nsp_reliable.o: src/vms_nsp_reliable.c src/vms_nsp_reliable.h | build
	$(CC) $(CFLAGS) -c $< -o $@
build/vms_mirror.o: src/vms_mirror.c src/vms_mirror.h | build
	$(CC) $(CFLAGS) -c $< -o $@
build/vms_mirror_server.o: src/vms_mirror_server.c src/vms_mirror_server.h src/vms_mirror.h src/vms_nsp_reliable.h src/vms_session.h | build
	$(CC) $(CFLAGS) -c $< -o $@
build/vms_serial.o: src/vms_serial.c src/vms_serial.h | build
	$(CC) $(CFLAGS) -c $< -o $@
build/vms_serial_x86.o: src/vms_serial_x86.c src/vms_serial.h | build
	$(CC) $(CFLAGS) -c $< -o $@
build/vms_qio.o: src/vms_qio.c src/vms_qio.h src/vms_ast.h src/vms_event.h | build
	$(CC) $(CFLAGS) -c $< -o $@
build/vms_channels.o: src/vms_channels.c src/vms_channels.h src/vms_qio.h | build
	$(CC) $(CFLAGS) -c $< -o $@
build/vms_exe_gettim.o: src/vms_exe_gettim.c src/vms_exe_gettim.h src/vms_userptr.h | build
	$(CC) $(EXEFLAGS) -c $< -o $@
build/vms_exe_setssf.o: src/vms_exe_setssf.c src/vms_exe_setssf.h src/vms_exe_gettim.h | build
	$(CC) $(EXEFLAGS) -c $< -o $@
build/vms_exe_setexv.o: src/vms_exe_setexv.c src/vms_exe_setexv.h src/vms_exe_gettim.h src/vms_userptr.h | build
	$(CC) $(EXEFLAGS) -c $< -o $@
build/vms_exe_getsyi.o: src/vms_exe_getsyi.c src/vms_exe_getsyi.h src/vms_exe_gettim.h src/vms_userptr.h | build
	$(CC) $(EXEFLAGS) -c $< -o $@
build/vms_exe_version.o: src/vms_exe_version.c src/vms_exe_version.h | build
	$(CC) $(EXEFLAGS) -c $< -o $@
build/vms_sysloavec.o: src/vms_sysloavec.c src/vms_sysloavec.h | build
	$(CC) $(CFLAGS) -c $< -o $@
build/vms_exe_event.o: src/vms_exe_event.c src/vms_exe_event.h src/vms_exe_gettim.h | build
	$(CC) $(EXEFLAGS) -c $< -o $@
build/vms_exe_schevt.o: src/vms_exe_schevt.c src/vms_exe_schevt.h src/vms_exe_event.h | build
	$(CC) $(EXEFLAGS) -c $< -o $@
build/vms_exe_setmod.o: src/vms_exe_setmod.c src/vms_exe_setmod.h | build
	$(CC) $(EXEFLAGS) -c $< -o $@
build/vms_exe_canevt.o: src/vms_exe_canevt.c src/vms_exe_canevt.h src/vms_exe_schevt.h | build
	$(CC) $(EXEFLAGS) -c $< -o $@
build/vms_exe_setstk.o: src/vms_exe_setstk.c src/vms_exe_setstk.h src/vms_exe_access.h | build
	$(CC) $(EXEFLAGS) -c $< -o $@
build/vms_exe_setprv.o: src/vms_exe_setprv.c src/vms_exe_setprv.h src/vms_exe_access.h | build
	$(CC) $(EXEFLAGS) -c $< -o $@
build/vms_exe_setpri.o: src/vms_exe_setpri.c src/vms_exe_setpri.h src/vms_exe_access.h | build
	$(CC) $(EXEFLAGS) -c $< -o $@
build/vms_exe_setpra.o: src/vms_exe_setpra.c src/vms_exe_setpra.h src/vms_exe_access.h | build
	$(CC) $(EXEFLAGS) -c $< -o $@
build/vms_exe_adjstk.o: src/vms_exe_adjstk.c src/vms_exe_adjstk.h src/vms_exe_access.h | build
	$(CC) $(EXEFLAGS) -c $< -o $@
build/vms_exe_derlmb.o: src/vms_exe_derlmb.c src/vms_exe_derlmb.h src/vms_exe_access.h | build
	$(CC) $(EXEFLAGS) -c $< -o $@
build/vms_exe_getpti.o: src/vms_exe_getpti.c src/vms_exe_getpti.h src/vms_exe_access.h | build
	$(CC) $(EXEFLAGS) -c $< -o $@
build/vms_exe_chgmod.o: src/vms_exe_chgmod.c src/vms_exe_chgmod.h | build
	$(CC) $(EXEFLAGS) -c $< -o $@
build/vms_exe_dclcmh.o: src/vms_exe_dclcmh.c src/vms_exe_dclcmh.h | build
	$(CC) $(EXEFLAGS) -c $< -o $@
build/vms_exe_sched.o: src/vms_exe_sched.c src/vms_exe_sched.h | build
	$(CC) $(EXEFLAGS) -c $< -o $@
build/vms_exe_nullproc.o: src/vms_exe_nullproc.c src/vms_exe_nullproc.h | build
	$(CC) $(EXEFLAGS) -c $< -o $@
build/executive-partial.o: build/vms_logintime.o build/vms_exe_gettim.o build/vms_exe_setssf.o build/vms_exe_setexv.o build/vms_exe_getsyi.o build/vms_exe_version.o build/vms_sysloavec.o build/vms_exe_event.o build/vms_exe_schevt.o build/vms_exe_setmod.o build/vms_exe_canevt.o build/vms_userptr.o build/vms_exe_setstk.o build/vms_exe_setprv.o build/vms_exe_setpri.o build/vms_exe_setpra.o build/vms_exe_adjstk.o build/vms_exe_derlmb.o build/vms_exe_getpti.o build/vms_exe_chgmod.o build/vms_exe_dclcmh.o build/vms_exe_sched.o build/vms_exe_nullproc.o build/vms_exe_astcon.o build/vms_exe_wait.o build/vms_exe_dclexh.o build/vms_exe_exit.o build/vms_exe_forcex.o build/vms_exe_setime.o build/vms_exe_cancel.o build/vms_exe_dassgn.o build/vms_exe_devalc.o build/vms_exe_purgws.o build/vms_exe_adjwsl.o build/vms_exe_setprt.o build/vms_exe_consolio.o build/vms_exe_scbvector.o build/vms_sys85.o build/vms_exe_memory85.o build/vms_rtl85.o build/vms_rtl85_tables.o build/vms_exe_image_end85.o
	$(LD) -m elf_i386 -r -o $@ $^
build/vms_userio.o: src/vms_userio.c src/vms_userio.h src/vms_userptr.h src/vms_qio.h | build
	$(CC) $(CFLAGS) -c $< -o $@
build/vms_userptr.o: src/vms_userptr.c src/vms_userptr.h | build
	$(CC) $(CFLAGS) -c $< -o $@
build/vms_session.o: src/vms_session.c src/vms_session.h src/vms_nsp_data.h | build
	$(CC) $(CFLAGS) -c $< -o $@
build/vms_ne2k_x86.o: src/vms_ne2k_x86.c src/vms_ne2k.h | build
	$(CC) $(CFLAGS) -c $< -o $@
build/vms_ata.o: src/vms_ata.c src/vms_ata.h | build
	$(CC) $(CFLAGS) -c $< -o $@
build/vms_ata_x86.o: src/vms_ata_x86.c src/vms_ata.h | build
	$(CC) $(CFLAGS) -c $< -o $@
build/vms_event.o: src/vms_event.c src/vms_event.h | build
	$(CC) $(CFLAGS) -c $< -o $@
build/vms_vmem.o: src/vms_vmem.c src/vms_vmem.h src/vms_phys.h src/vms_pmap.h | build
	$(CC) $(CFLAGS) -c $< -o $@
build/vms_ramcheck.o: src/vms_ramcheck.c src/vms_ramcheck.h | build
	$(CC) $(CFLAGS) -c $< -o $@
build/vms_phys.o: src/vms_phys.c src/vms_phys.h | build
	$(CC) $(CFLAGS) -c $< -o $@
build/kernel.o: src/kernel.c src/vms_ini_phv_swap.h src/vms_sys_executive_entry.h src/vms_ini_packet_queues.h src/vms_sysboot_pktmem.h src/vms_login_interact.h src/vms_logincmd.h src/vms_login_fileio.h src/vms_login_uart.h src/vms_hpwd.h src/vms_validate.h src/vms_logintime.h src/vms_dcl_exit.h src/vms_dcl_readrec.h src/vms_dcl_initial.h src/vms_dcl_command.h src/vms_rem_protocol.h src/vms_bootio.h src/vms_boot_files11.h src/vms_vmbargs.h src/vms_vmb_rpb.h src/vms_sysmount.h src/vms_sysboot_elf.h src/vms_dcl_goto.h src/vms_ast.h src/vms_astdel.h src/vms_postef.h src/vms_mutex.h src/vms_condition.h src/vms_qio_post.h | build
	$(CC) $(CFLAGS) -c $< -o $@
build/vms_ini_pc_bus.o: src/vms_ini_pc_bus.c src/vms_ini_pc_bus.h src/vms_iniloa_iomap.h | build
	$(CC) $(CFLAGS) -c $< -o $@
M125_DQ_SRCS=$(wildcard src/vms_dq_fdt_*.c)
M125_DQ_OBJS=$(patsubst src/%.c,build/%.o,$(M125_DQ_SRCS))
M197_DQ_CRB_SRCS=$(wildcard src/vms_dq_crb*.c)
M197_DQ_CRB_SRCS+=src/vms_v37_sched.c
M197_DQ_CRB_OBJS=$(patsubst src/%.c,build/%.o,$(M197_DQ_CRB_SRCS))
build/vms_v37_sched.o: src/vms_v37_sched.c src/vms_v37_sched.h src/vms_v37_pcb.h src/vms_v37_dyn_tags.h | build
	$(CC) $(CFLAGS) -c $< -o $@
M124_SWI_SRCS=$(wildcard src/vms_iopost_swi_*.c)
M124_SWI_OBJS=$(patsubst src/%.c,build/%.o,$(M124_SWI_SRCS))
# M246: kernel-owned initial IA-32 context frames share the interrupt ABI.
M124_SWI_OBJS+=build/vms_v37_ia32_context.o
build/vms_v37_ia32_context.o: src/vms_v37_ia32_context.h src/vms_pcb.h
build/kernel.elf: build/start.o build/vms_vmb_rpb.o build/vms_sysmount.o build/vms_vmbargs.o build/vms_filerwio.o build/vms_sysboot_map.o build/vms_sysboot_read.o build/vms_sysboot_elf.o build/vms_bootio.o build/vms_boot_files11.o build/vms_sysinit_names.o build/vms_initvec.o build/vms_booparam.o build/vms_boot_readprompt.o build/kernel.o build/vms_dcl_exit.o build/vms_dcl_status.o build/vms_dcl_if.o build/vms_dcl_inquire.o build/vms_dcl_on.o build/vms_dcl_intimages.o build/vms_dcl_goto.o build/vms_dcl_readrec.o build/vms_dcl_initial.o build/vms_dcl_command.o build/vms_dcl_descr.o build/vms_login_interact.o build/vms_logincmd.o build/vms_login_fileio.o build/vms_login_uart.o build/vms_hpwd.o build/vms_validate.o build/vms_uaf_v37.o build/vms_logintime.o build/vms_rtc_x86.o build/vms_hpwd_selftest.o build/vms_mutex.o build/vms_mutex_selftest.o build/vms_postef.o build/vms_postef_selftest.o build/vms_astdel.o build/vms_astdel_selftest.o build/vms_ast.o build/vms_condition.o build/vms_pmap.o build/vms_pcb.o build/vms_phys.o build/vms_ramcheck.o build/vms_vmem.o build/vms_event.o build/vms_ata.o build/vms_ata_x86.o build/vms_dq_crb.o build/vms_ne2k.o build/vms_ne2k_x86.o build/vms_decnet.o build/vms_rem_protocol.o build/vms_rem_protocol_selftest.o build/vms_nsp.o build/vms_nsp_link.o build/vms_session.o build/vms_nsp_data.o build/vms_nsp_reliable.o build/vms_mirror.o build/vms_mirror_server.o build/vms_serial.o build/vms_serial_x86.o build/vms_qio.o build/vms_channels.o build/vms_userptr.o build/vms_userio.o build/vms_kstack.o build/arch.o build/arch_asm.o build/vms_exe_gettim.o build/vms_exe_sched.o build/vms_ini_pc_bus.o $(M168_IOC_OBJS) linked.ld $(M124_SWI_OBJS) build/vms_dq_index_validate.o build/vms_dq_index_decode.o build/vms_dq_mfd_map.o build/vms_dq_mfd_validate.o build/vms_dq_index_location.o
	$(LD) -m elf_i386 -T linked.ld -o $@ build/start.o build/vms_vmb_rpb.o build/vms_sysmount.o build/vms_vmbargs.o build/vms_filerwio.o build/vms_sysboot_map.o build/vms_sysboot_read.o build/vms_sysboot_elf.o build/vms_bootio.o build/vms_boot_files11.o build/vms_sysinit_names.o build/vms_initvec.o build/vms_booparam.o build/vms_boot_readprompt.o build/kernel.o build/vms_dcl_exit.o build/vms_dcl_status.o build/vms_dcl_if.o build/vms_dcl_inquire.o build/vms_dcl_on.o build/vms_dcl_intimages.o build/vms_dcl_goto.o build/vms_dcl_readrec.o build/vms_dcl_initial.o build/vms_dcl_command.o build/vms_dcl_descr.o build/vms_login_interact.o build/vms_logincmd.o build/vms_login_fileio.o build/vms_login_uart.o build/vms_hpwd.o build/vms_validate.o build/vms_uaf_v37.o build/vms_logintime.o build/vms_rtc_x86.o build/vms_hpwd_selftest.o build/vms_mutex.o build/vms_mutex_selftest.o build/vms_postef.o build/vms_postef_selftest.o build/vms_astdel.o build/vms_astdel_selftest.o build/vms_ast.o build/vms_condition.o build/vms_pmap.o build/vms_pcb.o build/vms_phys.o build/vms_ramcheck.o build/vms_vmem.o build/vms_event.o build/vms_ata.o build/vms_ata_x86.o build/vms_dq_crb.o build/vms_ne2k.o build/vms_ne2k_x86.o build/vms_decnet.o build/vms_rem_protocol.o build/vms_rem_protocol_selftest.o build/vms_nsp.o build/vms_nsp_link.o build/vms_session.o build/vms_nsp_data.o build/vms_nsp_reliable.o build/vms_mirror.o build/vms_mirror_server.o build/vms_serial.o build/vms_serial_x86.o build/vms_qio.o build/vms_channels.o build/vms_userptr.o build/vms_userio.o build/vms_kstack.o build/arch.o build/arch_asm.o build/vms_exe_gettim.o build/vms_exe_sched.o build/vms_ini_pc_bus.o $(M168_IOC_OBJS) $(M124_SWI_OBJS) build/vms_dq_index_validate.o build/vms_dq_index_decode.o build/vms_dq_mfd_map.o build/vms_dq_mfd_validate.o build/vms_dq_index_location.o
build/kernel.bin: build/kernel.elf
	$(OBJCOPY) -O binary $< $@
portdisk.img: build/make_f11_disk build/install_sys_image build/sys.exe build/install_iniloa_image build/iniloa.exe build/install_sysloa386_image build/sysloa386.exe
	./build/make_f11_disk portdisk.img
	./build/install_sys_image portdisk.img build/sys.exe
	./build/install_iniloa_image portdisk.img build/iniloa.exe
	./build/install_sysloa386_image portdisk.img build/sysloa386.exe
vms37lab.ima: build/boot.bin build/kernel.bin
	python3 build_image.py

test: sysqioresidentcheck vmbargscheck initindexcheck initbitmapcheck initmfdcheck f11homecheck bootiocheck sysinitnamescheck initveccheck v37pcbcheck v37schedcheck v37phdcheck v37ia32ctxcheck booparamcheck bootreadpromptcheck vms37lab.ima portdisk.img dclexitcheck dclstatuscheck dclifcheck dclinquirecheck dcloncheck dclimagescheck dclgotocheck dcldescrcheck dclreadcheck dclinitialcheck dclcommandcheck remprotocolcheck hpwdcheck v37uafcheck validatecheck logintimecheck fileiocheck logincmdcheck interactcheck dqcrbcheck dqprivateioccheck dqwriteioccheck sysqioargscheck irpalloccheck
	python3 tools/gen_source_coverage.py
	python3 tests/test_image.py

	python3 tests/test_guest_layout.py
	python3 tests/test_privilege_layout.py
	python3 tests/test_scheduler_layout.py
	python3 tests/test_preempt_layout.py
	python3 tests/test_source_audit.py
	python3 tests/test_m6_layout.py
	python3 tests/test_m7_layout.py
	$(CC) -std=c11 -O2 -Wall -Wextra -Werror -Isrc src/vms_ast.c tests/test_ast_host.c -o build/ast_hosttest
	./build/ast_hosttest
	$(CC) -std=c11 -O2 -Wall -Wextra -Werror -Isrc src/vms_ast.c src/vms_astdel.c src/vms_astdel_selftest.c tests/test_astdel_host.c -o build/astdel_hosttest
	./build/astdel_hosttest
	$(CC) -std=c11 -O2 -Wall -Wextra -Werror -Isrc src/vms_mutex.c src/vms_mutex_selftest.c src/vms_pcb.c src/vms_ast.c tests/test_mutex_host.c -o build/mutex_hosttest
	./build/mutex_hosttest
	$(CC) -std=c11 -O2 -Wall -Wextra -Werror -Isrc src/vms_postef.c src/vms_postef_selftest.c src/vms_event.c src/vms_pcb.c src/vms_ast.c tests/test_postef_host.c -o build/postef_hosttest
	./build/postef_hosttest
	$(CC) -std=c11 -O2 -Wall -Wextra -Werror -Isrc src/vms_condition.c tests/test_condition_host.c -o build/condition_hosttest
	./build/condition_hosttest
	$(CC) -std=c11 -O2 -Wall -Wextra -Werror -Isrc src/vms_pmap.c tests/test_pmap_host.c -o build/pmap_hosttest
	./build/pmap_hosttest
	$(CC) -std=c11 -O2 -Wall -Wextra -Werror -Isrc src/vms_ast.c src/vms_pcb.c tests/test_pcb_host.c -o build/pcb_hosttest
	./build/pcb_hosttest
	$(CC) -std=c11 -O2 -Wall -Wextra -Werror -Isrc src/vms_kstack.c tests/test_kstack_host.c -o build/kstack_hosttest
	./build/kstack_hosttest
	$(CC) -std=c11 -O2 -Wall -Wextra -Werror -Isrc src/vms_phys.c tests/test_phys_host.c -o build/phys_hosttest
	./build/phys_hosttest
	$(CC) -std=c11 -O2 -Wall -Wextra -Werror -Isrc src/vms_vmem.c src/vms_phys.c tests/test_vmem_host.c -o build/vmem_hosttest
	./build/vmem_hosttest
	python3 tests/test_m8_layout.py
	$(CC) -std=c11 -O2 -Wall -Wextra -Werror -Isrc src/vms_ast.c src/vms_pcb.c src/vms_event.c tests/test_event_host.c -o build/event_hosttest
	./build/event_hosttest
	python3 tests/test_m9_layout.py
	$(CC) -std=c11 -O2 -Wall -Wextra -Werror -Isrc src/vms_ata.c tests/test_ata_host.c -o build/ata_hosttest
	./build/ata_hosttest
	python3 tests/test_ata_irq_layout.py
	python3 tests/test_disk_image.py
	python3 tests/test_m10_layout.py
	$(CC) -std=c11 -O2 -Wall -Wextra -Werror -Isrc src/vms_ne2k.c tests/test_ne2k_host.c -o build/ne2k_hosttest
	./build/ne2k_hosttest
	python3 tests/test_m11_layout.py
	python3 tests/test_m12_layout.py
	$(CC) -std=c11 -O2 -Wall -Wextra -Werror -Isrc src/vms_decnet.c tests/test_decnet_host.c -o build/decnet_hosttest
	./build/decnet_hosttest
	python3 tests/test_m13_layout.py
	$(CC) -std=c11 -O2 -Wall -Wextra -Werror -Isrc src/vms_decnet.c tests/test_hello_host.c -o build/hello_hosttest
	./build/hello_hosttest
	python3 tests/test_m14_layout.py
	$(CC) -std=c11 -O2 -Wall -Wextra -Werror -Isrc src/vms_decnet.c tests/test_router_host.c -o build/router_hosttest
	./build/router_hosttest
	python3 tests/test_m15_layout.py
	$(CC) -std=c11 -O2 -Wall -Wextra -Werror -Isrc src/vms_nsp.c src/vms_decnet.c tests/test_nsp_host.c -o build/nsp_hosttest
	./build/nsp_hosttest
	python3 tests/test_m16_layout.py
	$(CC) -std=c11 -O2 -Wall -Wextra -Werror -Isrc src/vms_nsp_link.c tests/test_nsp_link_host.c -o build/nsp_link_hosttest
	./build/nsp_link_hosttest
	python3 tests/test_m17_layout.py
	$(CC) -std=c11 -O2 -Wall -Wextra -Werror -Isrc src/vms_session.c src/vms_nsp.c src/vms_decnet.c tests/test_session_host.c -o build/session_hosttest
	./build/session_hosttest
	python3 tests/test_m18_layout.py
	$(CC) -std=c11 -O2 -Wall -Wextra -Werror -Isrc src/vms_nsp_data.c src/vms_decnet.c tests/test_nsp_data_host.c -o build/nsp_data_hosttest
	./build/nsp_data_hosttest
	python3 tests/test_m19_layout.py
	$(CC) -std=c11 -O2 -Wall -Wextra -Werror -Isrc src/vms_nsp_reliable.c tests/test_nsp_reliable_host.c -o build/nsp_reliable_hosttest
	./build/nsp_reliable_hosttest
	python3 tests/test_m20_layout.py
	$(CC) -std=c11 -O2 -Wall -Wextra -Werror -Isrc src/vms_mirror.c tests/test_mirror_host.c -o build/mirror_hosttest
	./build/mirror_hosttest
	python3 tests/test_m21_layout.py
	python3 tests/test_m21_boot.py
	$(CC) -std=c11 -O2 -Wall -Wextra -Werror -Isrc src/vms_mirror_server.c src/vms_mirror.c src/vms_session.c src/vms_nsp_data.c src/vms_nsp_reliable.c src/vms_nsp.c src/vms_decnet.c tests/test_mirror_server_host.c -o build/mirror_server_hosttest
	./build/mirror_server_hosttest
	python3 tests/test_m22_layout.py
	$(CC) -std=c11 -O2 -Wall -Wextra -Werror -Isrc src/vms_serial.c tests/test_serial_host.c -o build/serial_hosttest
	./build/serial_hosttest
	python3 tests/test_m23_layout.py
	$(CC) -std=c11 -O2 -Wall -Wextra -Werror -Isrc src/vms_nsp_link.c src/vms_nsp_reliable.c src/vms_mirror_server.c src/vms_mirror.c src/vms_session.c src/vms_nsp_data.c src/vms_nsp.c src/vms_decnet.c tests/test_two_node_host.c -o build/two_node_hosttest
	./build/two_node_hosttest
	$(CC) -std=c11 -O2 -Wall -Wextra -Werror -Isrc src/vms_decnet.c src/vms_nsp.c src/vms_nsp_data.c src/vms_session.c src/vms_mirror.c tests/test_protocol_fuzz_host.c -o build/protocol_fuzz_hosttest
	./build/protocol_fuzz_hosttest
	$(CC) -std=c11 -O2 -Wall -Wextra -Werror -Isrc src/vms_qio.c src/vms_event.c src/vms_ast.c tests/test_qio_host.c -o build/qio_hosttest
	./build/qio_hosttest
	python3 tests/test_m25_layout.py
	$(CC) -std=c11 -O2 -Wall -Wextra -Werror -Isrc src/vms_qio.c src/vms_channels.c src/vms_event.c src/vms_ast.c tests/test_channels_host.c -o build/channels_hosttest
	./build/channels_hosttest
	python3 tests/test_m26_layout.py
	$(CC) -std=c11 -O2 -Wall -Wextra -Werror -Isrc src/vms_userptr.c tests/test_userptr_host.c -o build/userptr_hosttest
	./build/userptr_hosttest
	python3 tests/test_m27_layout.py
	python3 tests/test_m28_layout.py
	python3 tests/test_m29_layout.py
	$(CC) -std=c11 -O2 -Wall -Wextra -Werror -Isrc src/vms_userptr.c tests/test_usercopy_host.c -o build/usercopy_hosttest
	./build/usercopy_hosttest
	python3 tests/test_m30_layout.py
	$(CC) -std=c11 -O2 -Wall -Wextra -Werror -Isrc src/vms_qio.c src/vms_channels.c src/vms_event.c src/vms_ast.c tests/test_qio_cancel_owner_host.c -o build/qio_cancel_owner_hosttest
	./build/qio_cancel_owner_hosttest
	python3 tests/test_m31_layout.py
	$(CC) -std=c11 -O2 -Wall -Wextra -Werror -Isrc src/vms_qio.c src/vms_channels.c src/vms_event.c src/vms_ast.c tests/test_teardown_host.c -o build/teardown_hosttest
	./build/teardown_hosttest
	python3 tests/test_m32_layout.py
	$(CC) -std=c11 -O2 -Wall -Wextra -Werror -Isrc src/vms_userptr.c tests/test_usercopyin_host.c -o build/usercopyin_hosttest
	./build/usercopyin_hosttest
	python3 tests/test_m33_layout.py
	$(CC) -std=c11 -O2 -Wall -Wextra -Werror -Isrc src/vms_userptr.c tests/test_usercopyv_host.c -o build/usercopyv_hosttest
	./build/usercopyv_hosttest
	python3 tests/test_m34_layout.py
	$(CC) -std=c11 -O2 -Wall -Wextra -Werror -Isrc src/vms_userptr.c src/vms_userio.c tests/test_userio_host.c -o build/userio_hosttest
	./build/userio_hosttest
	python3 tests/test_m35_layout.py
	$(CC) -std=c11 -O2 -Wall -Wextra -Werror -Isrc src/vms_serial.c tests/test_serial_rx_host.c -o build/serial_rx_hosttest
	./build/serial_rx_hosttest
	python3 tests/test_m36_layout.py
	python3 tests/test_m37_layout.py
	$(CC) -std=c11 -O2 -Wall -Wextra -Werror -Isrc src/vms_userptr.c tests/test_user_disjoint_host.c -o build/user_disjoint_hosttest
	./build/user_disjoint_hosttest
	python3 tests/test_m38_layout.py
	python3 tests/test_m39_layout.py
	python3 tests/test_m40_dma.py
	python3 tests/test_m41_a20.py
	python3 tools/boot16_probe.py
	python3 tools/kernel_image_verify.py
	python3 tests/test_m44_layout.py
	python3 tests/test_m47_qemu_manifest.py
	python3 tests/test_m49_stress_manifest.py

.PHONY: sysqioargscheck
sysqioargscheck: | build
	$(CC) -std=c11 -O2 -Wall -Wextra -Werror -Isrc src/vms_sysqio_args.c src/vms_sysqio_pcb.c src/vms_sysqio_irp_build.c src/vms_sysqio_irp_reserve.c src/vms_sysqio_normalize.c src/vms_sysqio_ast.c src/vms_sysqio_device_access.c src/vms_sysqio_ef.c src/vms_sysqio_iosb.c src/vms_qio_irp_pool.c src/vms_ini_packet_queues.c $(wildcard src/vms_dq_fdt_*.c) tests/test_sysqio_args_host.c -o build/sysqioargscheck
	./build/sysqioargscheck

.PHONY: sysqioresidentcheck
sysqioresidentcheck: build/sys.exe | build
	python3 tests/test_sysqio_resident.py build/sys.exe

clean:
	rm -rf build vms37lab.ima portdisk.img

smoke: vms37lab.ima portdisk.img
	python3 tools/qemu_smoke.py

.PHONY: bootcheck
bootcheck: vms37lab.ima
	python3 tools/boot16_probe.py

.PHONY: imagecheck
imagecheck: vms37lab.ima
	python3 tools/kernel_image_verify.py

.PHONY: m47check
m47check:
	python3 tests/test_m47_qemu_manifest.py
	python3 tests/test_m49_stress_manifest.py

.PHONY: m49check smoke-stress
m49check:
	python3 tests/test_m49_stress_manifest.py

smoke-stress: vms37lab.ima portdisk.img
	python3 tools/qemu_smoke.py --stress

.PHONY: hpwdcheck
hpwdcheck: | build
	$(CC) -std=c11 -O2 -Wall -Wextra -Werror -Isrc src/vms_hpwd.c tests/test_hpwd_host.c -o build/hpwd_hosttest
	./build/hpwd_hosttest

.PHONY: validatecheck
validatecheck: all
	$(CC) -std=c11 -O2 -Wall -Wextra -Werror -Isrc src/vms_validate.c src/vms_uaf_v37.c src/vms_hpwd.c tests/test_validate_host_m110.c -o build/validate_hosttest
	./build/validate_hosttest
	python3 tools/gen_source_coverage.py
	python3 tests/test_disk_image.py

.PHONY: logintimecheck
logintimecheck: all
	$(CC) -std=c11 -O2 -Wall -Wextra -Werror -Isrc src/vms_logintime.c tests/test_logintime_host.c -o build/logintime_hosttest
	./build/logintime_hosttest

.PHONY: fileiocheck
fileiocheck: | build
	$(CC) -std=c11 -O2 -Wall -Wextra -Werror -Isrc src/vms_login_fileio.c tests/test_login_fileio_host.c -o build/login_fileio_hosttest
	./build/login_fileio_hosttest
	python3 tools/gen_source_coverage.py

.PHONY: logincmdcheck
logincmdcheck: | build
	$(CC) -std=c11 -O2 -Wall -Wextra -Werror -Isrc src/vms_logincmd.c tests/test_logincmd_host.c -o build/logincmd_hosttest
	./build/logincmd_hosttest
	python3 tools/gen_source_coverage.py

.PHONY: interactcheck
interactcheck: portdisk.img | build
	$(CC) -std=c11 -O2 -Wall -Wextra -Werror -Isrc src/vms_login_interact.c src/vms_login_fileio.c src/vms_logincmd.c src/vms_validate.c src/vms_uaf_v37.c src/vms_hpwd.c tests/test_login_interact_host_m110.c -o build/login_interact_hosttest
	./build/login_interact_hosttest
	python3 tools/gen_source_coverage.py

.PHONY: v37uafcheck
v37uafcheck: | build
	$(CC) -std=c11 -O2 -Wall -Wextra -Werror -Isrc src/vms_uaf_v37.c tests/test_vms_uaf_v37_host.c -o build/v37uaf_hosttest
	./build/v37uaf_hosttest

.PHONY: dclexitcheck
dclexitcheck: | build
	$(CC) -std=c11 -O2 -Wall -Wextra -Werror -Isrc src/vms_dcl_exit.c tests/test_dcl_exit_host.c -o build/dcl_exit_hosttest
	./build/dcl_exit_hosttest

.PHONY: dclstatuscheck
dclstatuscheck: | build
	$(CC) -std=c11 -O2 -Wall -Wextra -Werror -Isrc src/vms_dcl_status.c tests/test_dcl_status_host.c -o build/dcl_status_hosttest
	./build/dcl_status_hosttest

.PHONY: dclifcheck
dclifcheck: | build
	$(CC) -std=c11 -O2 -Wall -Wextra -Werror -Isrc src/vms_dcl_if.c tests/test_dcl_if_host.c -o build/dcl_if_hosttest
	./build/dcl_if_hosttest

.PHONY: dclinquirecheck
dclinquirecheck: | build
	$(CC) -std=c11 -O2 -Wall -Wextra -Werror -Isrc src/vms_dcl_inquire.c tests/test_dcl_inquire_host.c -o build/dcl_inquire_hosttest
	./build/dcl_inquire_hosttest

.PHONY: dcloncheck dclimagescheck
dcloncheck: | build
	$(CC) -std=c11 -O2 -Wall -Wextra -Werror -Isrc src/vms_dcl_on.c src/vms_dcl_status.c tests/test_dcl_on_host.c -o build/dcl_on_hosttest
	./build/dcl_on_hosttest
dclimagescheck: | build
	$(CC) -std=c11 -O2 -Wall -Wextra -Werror -Isrc src/vms_dcl_intimages.c tests/test_dcl_intimages_host.c -o build/dcl_intimages_hosttest
	./build/dcl_intimages_hosttest

.PHONY: dclgotocheck
dclgotocheck: | build
	$(CC) -std=c11 -O2 -Wall -Wextra -Werror -Isrc src/vms_dcl_goto.c tests/test_dcl_goto_host.c -o build/dcl_goto_hosttest
	./build/dcl_goto_hosttest

.PHONY: dcldescrcheck
dcldescrcheck: | build
	$(CC) -std=c11 -O2 -Wall -Wextra -Werror -Isrc src/vms_dcl_descr.c tests/test_dcl_descr_host.c -o build/dcl_descr_hosttest
	./build/dcl_descr_hosttest

.PHONY: dclreadcheck
dclreadcheck: | build
	$(CC) -std=c11 -O2 -Wall -Wextra -Werror -Isrc src/vms_dcl_readrec.c tests/test_dcl_readrec_host.c -o build/dcl_readrec_hosttest
	./build/dcl_readrec_hosttest

.PHONY: dclinitialcheck
dclinitialcheck: | build
	$(CC) -std=c11 -O2 -Wall -Wextra -Werror -Isrc src/vms_dcl_initial.c tests/test_dcl_initial_host.c -o build/dcl_initial_hosttest
	./build/dcl_initial_hosttest

dclcommandcheck: | build
	$(CC) -std=c11 -O2 -Wall -Wextra -Werror -Isrc src/vms_dcl_command.c tests/test_dcl_command_host.c -o build/dcl_command_hosttest
	./build/dcl_command_hosttest

.PHONY: remprotocolcheck
remprotocolcheck: | build
	$(CC) -std=c11 -O2 -Wall -Wextra -Werror -Isrc src/vms_rem_protocol.c tests/test_rem_protocol_host.c -o build/rem_protocol_hosttest
	./build/rem_protocol_hosttest

.PHONY: bootreadpromptcheck
bootreadpromptcheck: | build
	$(CC) -std=c11 -O2 -Wall -Wextra -Werror -Isrc src/vms_boot_readprompt.c tests/test_boot_readprompt_host.c -o build/bootreadprompt_hosttest
	./build/bootreadprompt_hosttest

.PHONY: booparamcheck
booparamcheck: | build
	$(CC) -std=c11 -O2 -Wall -Wextra -Werror -Isrc src/vms_booparam.c tests/test_booparam_host.c -o build/booparam_hosttest
	./build/booparam_hosttest

.PHONY: initveccheck
initveccheck: | build
	$(CC) -std=c11 -O2 -Wall -Wextra -Werror -Isrc src/vms_initvec.c tests/test_initvec_host.c -o build/initvec_hosttest
	./build/initvec_hosttest

.PHONY: v37pcbcheck
v37pcbcheck: | build
	$(CC) -std=c11 -O2 -Wall -Wextra -Werror -Isrc src/vms_v37_pcb.c tests/test_v37_pcb_host.c -o build/v37pcbcheck
	./build/v37pcbcheck

.PHONY: v37schedcheck
v37schedcheck: | build
	$(CC) -std=c11 -O2 -Wall -Wextra -Werror -Isrc src/vms_v37_pcb.c src/vms_v37_sched.c tests/test_v37_sched_host.c -o build/v37schedcheck
	./build/v37schedcheck

.PHONY: v37phdcheck
v37phdcheck: | build
	$(CC) -std=c11 -O2 -Wall -Wextra -Werror -Isrc src/vms_v37_phd.c tests/test_v37_phd_host.c -o build/v37phdcheck
	./build/v37phdcheck
.PHONY: v37ia32ctxcheck
v37ia32ctxcheck: | build
	$(CC) -std=c11 -O2 -Wall -Wextra -Werror -Isrc src/vms_v37_ia32_context.c tests/test_v37_ia32_context_host.c -o build/v37ia32ctxcheck
	./build/v37ia32ctxcheck

.PHONY: sysinitnamescheck
sysinitnamescheck: | build
	$(CC) -std=c11 -O2 -Wall -Wextra -Werror -Isrc src/vms_sysinit_names.c tests/test_sysinit_names_host.c -o build/sysinit_names_hosttest
	./build/sysinit_names_hosttest

.PHONY: bootiocheck
bootiocheck: | build
	$(CC) -std=c11 -O2 -Wall -Wextra -Werror -Isrc src/vms_bootio.c src/vms_boot_files11.c src/vms_sysboot_map.c src/vms_sysboot_read.c tests/test_bootio_host.c -o build/bootio_hosttest
	./build/bootio_hosttest

.PHONY: initmfdcheck f11homecheck
initmfdcheck: src/vms_init_mfd.c src/vms_init_mfd.h tests/test_init_mfd_host.c | build
	$(CC) -std=c11 -O2 -Wall -Wextra -Werror -Isrc src/vms_init_mfd.c tests/test_init_mfd_host.c -o build/test_init_mfd_host
	./build/test_init_mfd_host
f11homecheck: src/vms_f11_home.c src/vms_f11_home.h tests/test_f11_home_host.c | build
	$(CC) -std=c11 -O2 -Wall -Wextra -Werror -Isrc src/vms_f11_home.c tests/test_f11_home_host.c -o build/test_f11_home_host
	./build/test_f11_home_host


build/make_f11_disk: tools/make_f11_disk.c src/vms_init_index.c src/vms_init_mfd.c src/vms_init_bitmap.c src/vms_f11_home.c src/vms_bootio.c src/vms_boot_files11.c | build
	$(CC) -std=c11 -O2 -Wall -Wextra -Werror -Isrc $^ -o $@
.PHONY: initindexcheck initbitmapcheck
initindexcheck: | build
	$(CC) -std=c11 -O2 -Wall -Wextra -Werror -Isrc src/vms_init_index.c src/vms_init_mfd.c src/vms_init_bitmap.c src/vms_f11_home.c src/vms_bootio.c src/vms_boot_files11.c tests/test_init_index_host.c -o build/test_init_index_host
	./build/test_init_index_host
initbitmapcheck: | build
	$(CC) -std=c11 -O2 -Wall -Wextra -Werror -Isrc src/vms_init_bitmap.c src/vms_f11_home.c tests/test_init_bitmap_host.c -o build/test_init_bitmap_host
	./build/test_init_bitmap_host

.PHONY: filerwiocheck
filerwiocheck: | build
	$(CC) -std=c11 -O2 -Wall -Wextra -Werror -Isrc src/vms_filerwio.c tests/test_filerwio_host.c -o build/test_filerwio_host
	./build/test_filerwio_host

.PHONY: sysbootmapcheck
sysbootmapcheck: | build
	$(CC) -std=c11 -O2 -Wall -Wextra -Werror -Isrc src/vms_sysboot_map.c src/vms_boot_files11.c src/vms_bootio.c tests/test_sysboot_map_host.c -o build/test_sysboot_map_host
	./build/test_sysboot_map_host

.PHONY: vmbargscheck
vmbargscheck: | build
	$(CC) -std=c11 -O2 -Wall -Wextra -Werror -Isrc src/vms_vmbargs.c tests/test_vmbargs_host.c -o build/vmbargs_hosttest
	./build/vmbargs_hosttest

.PHONY: vmbrpbcheck sysmountcheck
vmbrpbcheck: | build
	$(CC) -std=c11 -O2 -Wall -Wextra -Werror -Isrc src/vms_vmbargs.c src/vms_vmb_rpb.c tests/test_vmb_rpb_host.c -o build/vmb_rpb_hosttest
	./build/vmb_rpb_hosttest
sysmountcheck: | build
	$(CC) -std=c11 -O2 -Wall -Wextra -Werror -Isrc src/vms_sysmount.c tests/test_sysmount_host.c -o build/sysmount_hosttest
	./build/sysmount_hosttest
.PHONY: sysmountdiskcheck
sysmountdiskcheck: portdisk.img | build
	$(CC) -std=c11 -O2 -Wall -Wextra -Werror -Isrc src/vms_sysmount.c src/vms_boot_files11.c src/vms_bootio.c tests/test_sysmount_files11_host.c -o build/sysmount_disk_hosttest
	./build/sysmount_disk_hosttest portdisk.img

.PHONY: exeservicescheck
exeservicescheck: | build
	$(CC) -std=c11 -O2 -Wall -Wextra -Werror -Isrc src/vms_exe_gettim.c src/vms_exe_setssf.c src/vms_userptr.c tests/test_exe_services_host.c -o build/exe_services_hosttest
	./build/exe_services_hosttest

.PHONY: exeexvcheck
exeexvcheck: | build
	$(CC) -std=c11 -O2 -Wall -Wextra -Werror -Isrc src/vms_exe_setexv.c src/vms_userptr.c tests/test_exe_setexv_host.c -o build/exe_setexv_hosttest
	./build/exe_setexv_hosttest

.PHONY: exegetsyicheck
exegetsyicheck: | build
	$(CC) -std=c11 -O2 -Wall -Wextra -Werror -Isrc src/vms_exe_getsyi.c src/vms_userptr.c tests/test_exe_getsyi_host.c -o build/exe_getsyi_hosttest
	./build/exe_getsyi_hosttest
.PHONY: exeversioncheck
exeversioncheck: | build
	$(CC) -std=c11 -O2 -Wall -Wextra -Werror -Isrc src/vms_exe_version.c tests/test_exe_version_host.c -o build/exe_version_hosttest
	./build/exe_version_hosttest

.PHONY: sysloaveccheck
sysloaveccheck: | build
	$(CC) -std=c11 -O2 -Wall -Wextra -Werror -Isrc src/vms_sysloavec.c tests/test_sysloavec_host.c -o build/sysloavec_hosttest
	./build/sysloavec_hosttest

.PHONY: exeeventcheck
exeeventcheck: | build
	$(CC) -std=c11 -O2 -Wall -Wextra -Werror -Isrc src/vms_exe_event.c tests/test_exe_event_host.c -o build/exe_event_hosttest
	./build/exe_event_hosttest
.PHONY: exegetsyieventcheck
exegetsyieventcheck: | build
	$(CC) -std=c11 -O2 -Wall -Wextra -Werror -Isrc src/vms_exe_getsyi.c src/vms_exe_event.c src/vms_userptr.c tests/test_exe_getsyi_event_host.c -o build/exe_getsyi_event_hosttest
	./build/exe_getsyi_event_hosttest

.PHONY: exeschevtcheck
exeschevtcheck: | build
	$(CC) -std=c11 -O2 -Wall -Wextra -Werror -Isrc src/vms_exe_schevt.c src/vms_exe_event.c src/vms_postef.c src/vms_pcb.c src/vms_event.c src/vms_ast.c tests/test_exe_schevt_host.c -o build/exeschevt_hosttest
	./build/exeschevt_hosttest

.PHONY: execlockcheck
execlockcheck: | build
	$(CC) -std=c11 -O2 -Wall -Wextra -Werror -Isrc src/vms_exe_gettim.c src/vms_userptr.c tests/test_exe_clock_host.c -o build/execlock_hosttest
	./build/execlock_hosttest

.PHONY: exesetmodcheck execanevtcheck
exesetmodcheck: | build
	$(CC) -std=c11 -O2 -Wall -Wextra -Werror -Isrc src/vms_exe_setmod.c tests/test_exe_setmod_host.c -o build/exe_setmod_hosttest
	./build/exe_setmod_hosttest
execanevtcheck: | build
	$(CC) -std=c11 -O2 -Wall -Wextra -Werror -Isrc src/vms_exe_canevt.c src/vms_exe_schevt.c src/vms_exe_event.c tests/test_exe_canevt_host.c -o build/exe_canevt_hosttest
	./build/exe_canevt_hosttest

# VMS 3.7 executive services from seven complete original source modules.
EXE_M83_SERVICES=src/vms_exe_setstk.c src/vms_exe_setprv.c src/vms_exe_setpri.c src/vms_exe_setpra.c src/vms_exe_adjstk.c src/vms_exe_derlmb.c src/vms_exe_getpti.c
.PHONY: exeportbatchcheck
exeportbatchcheck: | build
	$(CC) -std=c11 -O2 -Wall -Wextra -Werror -Isrc $(EXE_M83_SERVICES) tests/test_exe_portbatch_host.c -o build/exe_portbatch_hosttest
	./build/exe_portbatch_hosttest

.PHONY: exeportcorecheck
exeportcorecheck: | build
	$(CC) -std=c11 -O2 -Wall -Wextra -Werror -Isrc src/vms_exe_chgmod.c src/vms_exe_dclcmh.c src/vms_exe_sched.c tests/test_exe_portcore_host.c -o build/exe_portcore_hosttest
	./build/exe_portcore_hosttest

EXE_M84_PROC=src/vms_exe_astcon.c src/vms_exe_wait.c src/vms_exe_dclexh.c src/vms_exe_exit.c src/vms_exe_forcex.c
.PHONY: exeproc84check
exeproc84check: | build
	$(CC) -std=c11 -O2 -Wall -Wextra -Werror -Isrc $(EXE_M84_PROC) src/vms_exe_event.c tests/test_exe_proc_services_host.c -o build/exeproc84_hosttest
	./build/exeproc84_hosttest

EXE_M84_IO=src/vms_exe_cancel.c src/vms_exe_dassgn.c src/vms_exe_devalc.c
.PHONY: exeio84check exetime84check
exeio84check: | build
	$(CC) -std=c11 -O2 -Wall -Wextra -Werror -Isrc $(EXE_M84_IO) tests/test_exe_io_services_host.c -o build/exeio84_hosttest
	./build/exeio84_hosttest
exetime84check: | build
	$(CC) -std=c11 -O2 -Wall -Wextra -Werror -Isrc src/vms_exe_setime.c src/vms_logintime.c src/vms_exe_gettim.c src/vms_exe_schevt.c src/vms_exe_event.c src/vms_userptr.c tests/test_exe_setime_host.c -o build/exetime84_hosttest
	./build/exetime84_hosttest

EXE_M84_MM=src/vms_exe_setprt.c src/vms_exe_adjwsl.c src/vms_exe_purgws.c
.PHONY: exemm84check
exemm84check: | build
	$(CC) -std=c11 -O2 -Wall -Wextra -Werror -Isrc $(EXE_M84_MM) tests/test_exe_memory_services_host.c -o build/exemm84_hosttest
	./build/exemm84_hosttest

EXE_M84_BOOT=src/vms_boo_actimage.c src/vms_rtf_readlbn.c src/vms_iogen_readdriv.c src/vms_boo_conio.c
.PHONY: exeboot84check
exeboot84check: | build
	$(CC) -std=c11 -O2 -Wall -Wextra -Werror -Isrc $(EXE_M84_BOOT) tests/test_boo_tools84_host.c -o build/exeboot84_hosttest
	./build/exeboot84_hosttest


# Native modules with original source-equivalent service bodies.
build/vms_exe_astcon.o: src/vms_exe_astcon.c src/vms_exe_astcon.h | build
	$(CC) $(EXEFLAGS) -c $< -o $@
build/vms_exe_wait.o: src/vms_exe_wait.c src/vms_exe_wait.h | build
	$(CC) $(EXEFLAGS) -c $< -o $@
build/vms_exe_dclexh.o: src/vms_exe_dclexh.c src/vms_exe_dclexh.h | build
	$(CC) $(EXEFLAGS) -c $< -o $@
build/vms_exe_exit.o: src/vms_exe_exit.c src/vms_exe_exit.h | build
	$(CC) $(EXEFLAGS) -c $< -o $@
build/vms_exe_forcex.o: src/vms_exe_forcex.c src/vms_exe_forcex.h | build
	$(CC) $(EXEFLAGS) -c $< -o $@
build/vms_exe_setime.o: src/vms_exe_setime.c src/vms_exe_setime.h | build
	$(CC) $(EXEFLAGS) -c $< -o $@
build/vms_exe_cancel.o: src/vms_exe_cancel.c src/vms_exe_cancel.h | build
	$(CC) $(EXEFLAGS) -c $< -o $@
build/vms_exe_dassgn.o: src/vms_exe_dassgn.c src/vms_exe_dassgn.h | build
	$(CC) $(EXEFLAGS) -c $< -o $@
build/vms_exe_devalc.o: src/vms_exe_devalc.c src/vms_exe_devalc.h | build
	$(CC) $(EXEFLAGS) -c $< -o $@
build/vms_exe_purgws.o: src/vms_exe_purgws.c src/vms_exe_purgws.h | build
	$(CC) $(EXEFLAGS) -c $< -o $@
build/vms_exe_adjwsl.o: src/vms_exe_adjwsl.c src/vms_exe_adjwsl.h | build
	$(CC) $(EXEFLAGS) -c $< -o $@
build/vms_exe_setprt.o: src/vms_exe_setprt.c src/vms_exe_setprt.h | build
	$(CC) $(EXEFLAGS) -c $< -o $@
build/vms_exe_consolio.o: src/vms_exe_consolio.c src/vms_exe_consolio.h | build
	$(CC) $(EXEFLAGS) -c $< -o $@
build/vms_exe_scbvector.o: src/vms_exe_scbvector.c src/vms_exe_scbvector.h | build
	$(CC) $(EXEFLAGS) -c $< -o $@
build/vms_boo_actimage.o: src/vms_boo_actimage.c src/vms_boo_actimage.h | build
	$(CC) $(CFLAGS) -c $< -o $@
build/vms_rtf_readlbn.o: src/vms_rtf_readlbn.c src/vms_rtf_readlbn.h | build
	$(CC) $(CFLAGS) -c $< -o $@
build/vms_iogen_readdriv.o: src/vms_iogen_readdriv.c src/vms_iogen_readdriv.h | build
	$(CC) $(CFLAGS) -c $< -o $@
build/vms_boo_conio.o: src/vms_boo_conio.c src/vms_boo_conio.h | build
	$(CC) $(CFLAGS) -c $< -o $@
build/vms_boo_readprompt.o: src/vms_boo_readprompt.c src/vms_boo_readprompt.h | build
	$(CC) $(CFLAGS) -c $< -o $@
build/vms_boo_sysgenmn.o: src/vms_boo_sysgenmn.c src/vms_boo_sysgenmn.h | build
	$(CC) $(CFLAGS) -c $< -o $@
build/boot-utilities-partial.o: build/vms_boo_t58bootio.o build/vms_boo_shodev.o build/vms_boo_actimage.o build/vms_rtf_readlbn.o build/vms_iogen_readdriv.o build/vms_boo_conio.o build/vms_boo_readprompt.o build/vms_boo_sysgenmn.o
	$(LD) -m elf_i386 -r -o $@ $^

.PHONY: execonsole84check exescb84check exereadprompt84check exesysgen84check
execonsole84check: | build
	$(CC) -std=c11 -O2 -Wall -Wextra -Werror -Isrc src/vms_exe_consolio.c tests/test_exe_consolio_host.c -o build/test_exe_consolio_host
	./build/test_exe_consolio_host
exescb84check: | build
	$(CC) -std=c11 -O2 -Wall -Wextra -Werror -Isrc src/vms_exe_scbvector.c tests/test_exe_scbvector_host.c -o build/test_exe_scbvector_host
	./build/test_exe_scbvector_host
exereadprompt84check: | build
	$(CC) -std=c11 -O2 -Wall -Wextra -Werror -Isrc src/vms_boo_readprompt.c tests/test_boo_readprompt_host.c -o build/test_boo_readprompt_host
	./build/test_boo_readprompt_host
exesysgen84check: | build
	$(CC) -std=c11 -O2 -Wall -Wextra -Werror -Isrc src/vms_boo_sysgenmn.c tests/test_boo_sysgenmn_host.c -o build/test_boo_sysgenmn_host
	./build/test_boo_sysgenmn_host

.PHONY: exet5884check exeshodev84check
build/vms_boo_t58bootio.o: src/vms_boo_t58bootio.c src/vms_boo_t58bootio.h | build
	$(CC) $(CFLAGS) -c $< -o $@
exet5884check: | build
	$(CC) -std=c11 -O2 -Wall -Wextra -Werror -Isrc src/vms_boo_t58bootio.c tests/test_boo_t58bootio_host.c -o build/test_boo_t58bootio_host
	./build/test_boo_t58bootio_host
build/vms_boo_shodev.o: src/vms_boo_shodev.c src/vms_boo_shodev.h | build
	$(CC) $(CFLAGS) -c $< -o $@
exeshodev84check: | build
	$(CC) -std=c11 -O2 -Wall -Wextra -Werror -Isrc src/vms_boo_shodev.c tests/test_boo_shodev_host.c -o build/test_boo_shodev_host
	./build/test_boo_shodev_host

# LOGINOUT supervisor protection and image replacement.
build/vms_lgi_protcli.o: src/vms_lgi_protcli.c src/vms_lgi_protcli.h src/vms_exe_setprt.h | build
	$(CC) $(CFLAGS) -c $< -o $@
build/login-utilities-partial.o: build/vms_lgi_protcli.o
	$(LD) -m elf_i386 -r -o $@ $^
.PHONY: exelgi84check
exelgi84check: | build
	$(CC) -std=c11 -O2 -Wall -Wextra -Werror -Isrc src/vms_lgi_protcli.c tests/test_lgi_protcli_host.c -o build/test_lgi_protcli_host
	./build/test_lgi_protcli_host

# VMS 3.7 RTL string, bit, and packed-decimal translation services.
build/vms_rtl85.o: src/vms_rtl85.c src/vms_rtl85.h | build
	$(CC) $(CFLAGS) -c $< -o $@
build/vms_rtl85_tables.o: src/vms_rtl85_tables.c src/vms_rtl85.h | build
	$(CC) $(CFLAGS) -c $< -o $@
build/rtl-library-partial.o: build/vms_rtl85.o build/vms_rtl85_tables.o
	$(LD) -m elf_i386 -r -o $@ $^
.PHONY: rtl85check
rtl85check: | build
	$(CC) -std=c11 -O2 -Wall -Wextra -Werror -I src src/vms_rtl85.c src/vms_rtl85_tables.c tests/test_rtl85_host.c -o build/rtl85check
	./build/rtl85check

build/vms_sys85.o: src/vms_sys85.c src/vms_sys85.h src/vms_rtl85.h | build
	$(CC) $(EXEFLAGS) -c $< -o $@
.PHONY: sys85check
sys85check: | build
	$(CC) -std=c11 -O2 -Wall -Wextra -Werror -I src src/vms_sys85.c src/vms_rtl85.c src/vms_rtl85_tables.c tests/test_sys85_host.c -o build/sys85check
	./build/sys85check

build/vms_exe_memory85.o: src/vms_exe_memory85.c src/vms_exe_memory85.h src/vms_userptr.h | build
	$(CC) $(EXEFLAGS) -c $< -o $@
.PHONY: exe85memorycheck
exe85memorycheck: | build
	$(CC) -std=c11 -O2 -Wall -Wextra -Werror -I src src/vms_exe_memory85.c src/vms_userptr.c tests/test_exe85_memory_host.c -o build/exe85memorycheck
	./build/exe85memorycheck

build/vms_exe_image_end85.o: src/vms_exe_image_end85.c src/vms_exe_image_end85.h | build
	$(CC) $(EXEFLAGS) -c $< -o $@
.PHONY: imageend85check
imageend85check: | build
	$(CC) -std=c11 -O2 -Wall -Wextra -Werror -fno-pie -no-pie -Isrc src/vms_exe_image_end85.c tests/test_image_end85_host.c -o build/imageend85check
	./build/imageend85check

# F11A and F11B ODS-1/2 ACP resident counterparts.
build/vms_f11_acp86.o: src/vms_f11_acp86.c src/vms_f11_acp86.h | build
	$(CC) $(EXEFLAGS) -c $< -o $@
build/vms_f11_acp87.o: src/vms_f11_acp87.c src/vms_f11_acp87.h src/vms_f11_acp86.h | build
	$(CC) $(EXEFLAGS) -c $< -o $@
build/files11-acp-partial.o: build/vms_f11_acp86.o build/vms_f11_acp87.o
	$(LD) -m elf_i386 -r -o $@ $^
.PHONY: files1186check
files1186check: portdisk.img | build
	$(CC) -std=c11 -O2 -Wall -Wextra -Werror -Isrc src/vms_f11_acp86.c src/vms_logintime.c tests/test_f11_acp86_host.c -o build/files1186check
	./build/files1186check portdisk.img

.PHONY: files1187check
files1187check: portdisk.img build/files11-acp-partial.o | build
	$(CC) -std=c11 -O2 -Wall -Wextra -Werror -Isrc src/vms_f11_acp87.c src/vms_f11_acp86.c src/vms_logintime.c tests/test_f11_acp87_host.c -o build/files1187check
	./build/files1187check portdisk.img

# M88: genuine, independently linked native SYS.EXE and Files-11 installation.
build/vms_sys_executive_entry.o: src/vms_sys_executive_entry.c src/vms_sys_executive_entry.h src/vms_sysloavec.h src/vms_sysloa386.h src/vms_exe_gettim.h src/vms_ini_packet_queues.h src/vms_sysboot_pktmem.h src/vms_ini_lockdb.h src/vms_ini_procvec.h src/vms_ini_phv_swap.h src/vms_ini_mpw.h src/vms_ini_pflvec.h src/vms_ini_syswcb.h src/vms_ini_log_templates.h src/vms_v37_dyn_tags.h src/vms_v37_pcb.h src/vms_ini_syslog.h src/vms_ini_bootdev_scan.h src/vms_ini_dpt_templates.h src/vms_sys85.h src/vms_qio_post.h src/vms_qio_irp_pool.h src/vms_dq_bootread.h src/vms_dqddt.h src/vms_dq_crb.h src/vms_ini_devicedb.h src/vms_ioc_phys.h | build
	$(CC) $(CFLAGS) -c $< -o $@
build/vms_sys_executive_entry.o: src/vms_v37_sched.h
build/vms_sysboot_pktmem.o: src/vms_sysboot_pktmem.c src/vms_sysboot_pktmem.h src/vms_booparam.h | build
	$(CC) $(CFLAGS) -c $< -o $@
build/vms_ini_packet_queues.o: src/vms_ini_packet_queues.c src/vms_ini_packet_queues.h | build
	$(CC) $(CFLAGS) -c $< -o $@
build/vms_ini_lockdb.o: src/vms_ini_lockdb.c src/vms_ini_lockdb.h | build
	$(CC) $(CFLAGS) -c $< -o $@
build/vms_ini_procvec.o: src/vms_ini_procvec.c src/vms_ini_procvec.h | build
	$(CC) $(CFLAGS) -c $< -o $@
build/vms_ini_phv_swap.o: src/vms_ini_phv_swap.c src/vms_ini_phv_swap.h | build
	$(CC) $(CFLAGS) -c $< -o $@
build/vms_ini_mpw.o: src/vms_ini_mpw.c src/vms_ini_mpw.h | build
	$(CC) $(CFLAGS) -c $< -o $@

# M118: original DEVICEDAT.MAR DDB/UCB pointer graph in real SYS.EXE.
M118_DB_SRCS=src/vms_ini_devicedb.c src/vms_ini_ddb_boot.c src/vms_ini_ddb_opa.c src/vms_ini_ddb_mb.c src/vms_ini_ddb_nl.c src/vms_ini_ucb_boot.c src/vms_ini_ucb_opa.c src/vms_ini_ucb_mb1.c src/vms_ini_ucb_mb2.c src/vms_ini_ucb_nl.c
M118_DB_OBJS=$(patsubst src/%.c,build/%.o,$(M118_DB_SRCS))
$(M118_DB_OBJS): build/%.o: src/%.c src/vms_ini_devicedb.h | build
	$(CC) $(CFLAGS) -c $< -o $@
.PHONY: inidevicedbcheck
inidevicedbcheck: $(M118_DB_OBJS) | build
	$(CC) -std=c11 -O2 -g -Wall -Wextra -Werror -fsanitize=undefined -fno-sanitize-recover=undefined -Isrc $(M118_DB_SRCS) tests/test_ini_devicedb_host.c -o build/inidevicedbcheck
	./build/inidevicedbcheck

# M119: original CONINTDSP source-defined port control on real UCB bytes.
M119_CON_SRCS=src/vms_con_port.c src/vms_con_xoff.c src/vms_con_xon.c src/vms_con_stop.c src/vms_con_stop2.c src/vms_con_abort.c src/vms_con_resume.c
M119_CON_OBJS=$(patsubst src/%.c,build/%.o,$(M119_CON_SRCS))
M120_IOC_SRCS=$(wildcard src/vms_ioc_*.c)
M120_IOC_OBJS=$(patsubst src/%.c,build/%.o,$(M120_IOC_SRCS))
M121_QIO_SRCS=$(wildcard src/vms_qio_*.c)
M121_QIO_OBJS=$(patsubst src/%.c,build/%.o,$(M121_QIO_SRCS))
# M218/M250/M251: native SYSQIO transaction helpers, now bound to the
# source-layout PCB quota and local-event fields in resident SYS.EXE.
M218_SYSQIO_SRCS=$(wildcard src/vms_sysqio_*.c)
M218_SYSQIO_OBJS=$(patsubst src/%.c,build/%.o,$(M218_SYSQIO_SRCS))
# The resident SYS.EXE link recipe consumes M121_QIO_OBJS.
M121_QIO_OBJS+=$(M218_SYSQIO_OBJS)
M122_NL_DDT_SRCS=$(wildcard src/vms_nl_ddt_*.c)
M122_NL_DDT_OBJS=$(patsubst src/%.c,build/%.o,$(M122_NL_DDT_SRCS))
M126_DQ_READ_SRCS=$(wildcard src/vms_dq_bootread_*.c)
M126_DQ_READ_OBJS=$(patsubst src/%.c,build/%.o,$(M126_DQ_READ_SRCS))
M127_FIN_SRCS=$(wildcard src/vms_ini_fin_*.c)
M127_FIN_OBJS=$(patsubst src/%.c,build/%.o,$(M127_FIN_SRCS))
M128_LOG_SRCS=$(wildcard src/vms_log_*.c)
M128_LOG_OBJS=$(patsubst src/%.c,build/%.o,$(M128_LOG_SRCS))
M129_DEVLOG_SRCS=$(wildcard src/vms_devlog_*.c)
M129_DEVLOG_OBJS=$(patsubst src/%.c,build/%.o,$(M129_DEVLOG_SRCS))
M130_DQDDT_SRCS=$(wildcard src/vms_dqddt_*.c)
M130_DQDDT_OBJS=$(patsubst src/%.c,build/%.o,$(M130_DQDDT_SRCS))
M131_RELOC_SRCS=$(wildcard src/vms_relocdrv_*.c)
M131_RELOC_OBJS=$(patsubst src/%.c,build/%.o,$(M131_RELOC_SRCS))
M134_DDTRELOC_SRCS=$(wildcard src/vms_ddtreloc_*.c)
M134_DDTRELOC_OBJS=$(patsubst src/%.c,build/%.o,$(M134_DDTRELOC_SRCS))
M123_IOPOST_SRCS=$(filter-out $(M124_SWI_SRCS),$(wildcard src/vms_iopost_*.c))
M123_IOPOST_OBJS=$(patsubst src/%.c,build/%.o,$(M123_IOPOST_SRCS))
$(M124_SWI_OBJS): build/%.o: src/%.c src/vms_iopost_swi.h | build
	$(CC) $(CFLAGS) -c $< -o $@
.PHONY: swiopostcheck
swiopostcheck: $(M124_SWI_OBJS) | build
	$(CC) -std=c11 -O2 -g -Wall -Wextra -Werror -fsanitize=undefined -fno-sanitize-recover=undefined -Isrc $(M124_SWI_SRCS) tests/test_iopost_swi_host.c -o build/swiopostcheck
	./build/swiopostcheck
$(M119_CON_OBJS): build/%.o: src/%.c src/vms_con_port.h | build
	$(CC) $(CFLAGS) -c $< -o $@
.PHONY: conportcheck
conportcheck: $(M119_CON_OBJS) | build
	$(CC) -std=c11 -O2 -g -Wall -Wextra -Werror -fsanitize=undefined -fno-sanitize-recover=undefined -Isrc $(M119_CON_SRCS) tests/test_con_port_host.c -o build/conportcheck
	./build/conportcheck

# M137: FIL$MOUNT INDEXF.SYS resident native DQ physical root FH2.
M137_INDEX_SRCS=$(wildcard src/vms_dq_index_*.c)
M137_INDEX_OBJS=$(patsubst src/%.c,build/%.o,$(M137_INDEX_SRCS))
$(M137_INDEX_OBJS): build/%.o: src/%.c src/vms_dq_index_header.h | build
	$(CC) $(CFLAGS) -c $< -o $@
.PHONY: dqindexcheck
dqindexcheck: $(M137_INDEX_OBJS) portdisk.img | build
	$(CC) -std=c11 -O2 -g -Wall -Wextra -Werror -fsanitize=undefined -fno-sanitize-recover=undefined -Isrc $(M137_INDEX_SRCS) $(M126_DQ_READ_SRCS) $(M125_DQ_SRCS) $(M118_DB_SRCS) src/vms_qio_irp_prepare.c tests/test_dq_index_host.c -o build/dqindexcheck
	./build/dqindexcheck
# M138: source FIL$READVBN / FIL$RDCHKFILHDR INDEXF.SYS mapped MFD.
M138_MFD_SRCS=$(wildcard src/vms_dq_mfd_*.c)
M138_MFD_OBJS=$(patsubst src/%.c,build/%.o,$(M138_MFD_SRCS))
$(M138_MFD_OBJS): build/%.o: src/%.c src/vms_dq_mfd_header.h | build
	$(CC) $(CFLAGS) -c $< -o $@
.PHONY: dqmfdcheck
dqmfdcheck: $(M138_MFD_OBJS) portdisk.img | build
	$(CC) -std=c11 -O2 -g -Wall -Wextra -Werror -fsanitize=undefined -fno-sanitize-recover=undefined -Isrc $(M138_MFD_SRCS) $(M137_INDEX_SRCS) $(M126_DQ_READ_SRCS) $(M125_DQ_SRCS) $(M118_DB_SRCS) src/vms_qio_irp_prepare.c tests/test_dq_mfd_host.c -o build/dqmfdcheck
	./build/dqmfdcheck
# M139: native source FIL$READVBN/FIL$FINDFILID directory traversal.
M139_DIR_SRCS=$(wildcard src/vms_dq_dir_*.c)
M139_DIR_OBJS=$(patsubst src/%.c,build/%.o,$(M139_DIR_SRCS))
$(M139_DIR_OBJS): build/%.o: src/%.c src/vms_dq_dir.h | build
	$(CC) $(CFLAGS) -c $< -o $@
.PHONY: dqdircheck
dqdircheck: $(M139_DIR_OBJS) portdisk.img | build
	$(CC) -std=c11 -O2 -g -Wall -Wextra -Werror -fsanitize=undefined -fno-sanitize-recover=undefined -Isrc $(M139_DIR_SRCS) $(M138_MFD_SRCS) $(M137_INDEX_SRCS) $(M126_DQ_READ_SRCS) $(M125_DQ_SRCS) $(M118_DB_SRCS) src/vms_qio_irp_prepare.c tests/test_dq_dir_host.c -o build/dqdircheck
	./build/dqdircheck
# M140: source FIL$READVBN physical SYS.EXE FH2 and IA-32 file image.
M140_IMAGE_SRCS=$(wildcard src/vms_dq_image_*.c)
M140_IMAGE_OBJS=$(patsubst src/%.c,build/%.o,$(M140_IMAGE_SRCS))
$(M140_IMAGE_OBJS): build/%.o: src/%.c src/vms_dq_imageprobe.h | build
	$(CC) $(CFLAGS) -c $< -o $@
.PHONY: dqimagecheck
dqimagecheck: $(M140_IMAGE_OBJS) portdisk.img build/sys.exe | build
	$(CC) -std=c11 -O2 -g -Wall -Wextra -Werror -fsanitize=undefined -fno-sanitize-recover=undefined -Isrc $(M140_IMAGE_SRCS) $(M139_DIR_SRCS) $(M138_MFD_SRCS) $(M137_INDEX_SRCS) $(M126_DQ_READ_SRCS) $(M125_DQ_SRCS) $(M118_DB_SRCS) src/vms_qio_irp_prepare.c tests/test_dq_image_host.c -o build/dqimagecheck
	./build/dqimagecheck
# M144: separately loaded SYSLOA386 and INILOA native image disk mapping.
M144_AUX_SRCS=$(wildcard src/vms_dq_aux_*.c)
M144_AUX_OBJS=$(patsubst src/%.c,build/%.o,$(M144_AUX_SRCS))
$(M144_AUX_OBJS): build/%.o: src/%.c src/vms_dq_aux_images.h | build
	$(CC) $(CFLAGS) -c $< -o $@
.PHONY: dqauxcheck
dqauxcheck: $(M144_AUX_OBJS) portdisk.img build/sysloa386.exe build/iniloa.exe | build
	$(CC) -std=c11 -O2 -g -Wall -Wextra -Werror -fsanitize=undefined -fno-sanitize-recover=undefined -Isrc $(M144_AUX_SRCS) $(M140_IMAGE_SRCS) $(M139_DIR_SRCS) $(M138_MFD_SRCS) $(M137_INDEX_SRCS) $(M126_DQ_READ_SRCS) $(M125_DQ_SRCS) $(M118_DB_SRCS) src/vms_qio_irp_prepare.c tests/test_dq_aux_host.c -o build/dqauxcheck
	./build/dqauxcheck


# M146: byte-granular FIL$READVBN across FH2 physical disk extents.
M146_RANGE_SRCS=$(wildcard src/vms_f11_range_*.c)
M146_RANGE_OBJS=$(patsubst src/%.c,build/%.o,$(M146_RANGE_SRCS))
$(M146_RANGE_OBJS): build/%.o: src/%.c src/vms_f11_range.h src/vms_f11_range_private.h | build
	$(CC) $(CFLAGS) -c $< -o $@
.PHONY: f11rangecheck
f11rangecheck: $(M146_RANGE_OBJS) portdisk.img build/sys.exe | build
	$(CC) -std=c11 -O2 -g -Wall -Wextra -Werror -fsanitize=undefined -fno-sanitize-recover=undefined -Isrc $(M146_RANGE_SRCS) $(M140_IMAGE_SRCS) $(M139_DIR_SRCS) $(M138_MFD_SRCS) $(M137_INDEX_SRCS) $(M126_DQ_READ_SRCS) $(M125_DQ_SRCS) $(M118_DB_SRCS) src/vms_qio_irp_prepare.c tests/test_f11_range_host.c -o build/f11rangecheck
	./build/f11rangecheck

# M147-M149: bounded native FIL$FINDFILID/RDCHKFILHDR/READVBN private path.
M149_BOOTPATH_SRCS=$(wildcard src/vms_f11_boot_*.c)
M149_BOOTPATH_OBJS=$(patsubst src/%.c,build/%.o,$(M149_BOOTPATH_SRCS))
$(M149_BOOTPATH_OBJS): build/%.o: src/%.c src/vms_f11_bootpath.h | build
	$(CC) $(CFLAGS) -c $< -o $@
.PHONY: f11bootpathcheck
f11bootpathcheck: portdisk.img $(M149_BOOTPATH_OBJS) | build
	$(CC) -std=c11 -O2 -g -Wall -Wextra -Werror -fsanitize=undefined -fno-sanitize-recover=undefined -Isrc $(M149_BOOTPATH_SRCS) $(M146_RANGE_SRCS) $(M140_IMAGE_SRCS) $(M139_DIR_SRCS) $(M138_MFD_SRCS) $(M137_INDEX_SRCS) $(M126_DQ_READ_SRCS) $(M125_DQ_SRCS) $(M118_DB_SRCS) src/vms_qio_irp_prepare.c tests/test_f11_bootpath.c -o build/f11bootpathcheck
	./build/f11bootpathcheck
# M149 private SYS.EXE directory version enumeration; not a public ACP.
M149_BOOTDIR_SRCS=$(wildcard src/vms_f11_bootdir_*.c)
M149_BOOTDIR_OBJS=$(patsubst src/%.c,build/%.o,$(M149_BOOTDIR_SRCS))
$(M149_BOOTDIR_OBJS): build/%.o: src/%.c src/vms_f11_bootdir.h | build
	$(CC) $(CFLAGS) -c $< -o $@
.PHONY: f11bootdircheck
f11bootdircheck: portdisk.img $(M149_BOOTDIR_OBJS) | build
	$(CC) -std=c11 -O2 -g -Wall -Wextra -Werror -fsanitize=undefined -fno-sanitize-recover=undefined -Isrc $(M149_BOOTDIR_SRCS) $(M149_BOOTPATH_SRCS) $(M146_RANGE_SRCS) $(M140_IMAGE_SRCS) $(M139_DIR_SRCS) $(M138_MFD_SRCS) $(M137_INDEX_SRCS) $(M126_DQ_READ_SRCS) $(M125_DQ_SRCS) $(M118_DB_SRCS) src/vms_qio_irp_prepare.c tests/test_f11_bootdir.c -o build/f11bootdircheck
	./build/f11bootdircheck
.PHONY: f11bootfragcheck
f11bootfragcheck: portdisk.img $(M149_BOOTDIR_OBJS) | build
	$(CC) -std=c11 -O2 -g -Wall -Wextra -Werror -fsanitize=undefined -fno-sanitize-recover=undefined -Isrc $(M149_BOOTDIR_SRCS) $(M149_BOOTPATH_SRCS) $(M146_RANGE_SRCS) $(M140_IMAGE_SRCS) $(M139_DIR_SRCS) $(M138_MFD_SRCS) $(M137_INDEX_SRCS) $(M126_DQ_READ_SRCS) $(M125_DQ_SRCS) $(M118_DB_SRCS) src/vms_qio_irp_prepare.c tests/test_f11_bootdir_fragmented.c -o build/f11bootfragcheck
	./build/f11bootfragcheck
# M150-M152: source FIL$RDCHKFILHDR extension FH2 chains, no live ACP.
M152_CHAIN_SRCS=$(wildcard src/vms_f11_chain_*.c)
M152_CHAIN_OBJS=$(patsubst src/%.c,build/%.o,$(M152_CHAIN_SRCS))
$(M152_CHAIN_OBJS): build/%.o: src/%.c src/vms_f11_chain.h | build
	$(CC) $(CFLAGS) -c $< -o $@
.PHONY: f11chaincheck f11chainextcheck
f11chaincheck: portdisk.img $(M152_CHAIN_OBJS) | build
	$(CC) -std=c11 -O2 -g -Wall -Wextra -Werror -fsanitize=undefined -fno-sanitize-recover=undefined -Isrc $(M152_CHAIN_SRCS) $(M149_BOOTDIR_SRCS) $(M149_BOOTPATH_SRCS) $(M146_RANGE_SRCS) $(M140_IMAGE_SRCS) $(M139_DIR_SRCS) $(M138_MFD_SRCS) $(M137_INDEX_SRCS) $(M126_DQ_READ_SRCS) $(M125_DQ_SRCS) $(M118_DB_SRCS) src/vms_boot_files11.c src/vms_bootio.c src/vms_qio_irp_prepare.c tests/test_f11_chain_host.c -o build/f11chaincheck
	./build/f11chaincheck
f11chainextcheck: portdisk.img $(M152_CHAIN_OBJS) | build
	$(CC) -std=c11 -O2 -g -Wall -Wextra -Werror -fsanitize=undefined -fno-sanitize-recover=undefined -Isrc $(M152_CHAIN_SRCS) $(M149_BOOTDIR_SRCS) $(M149_BOOTPATH_SRCS) $(M146_RANGE_SRCS) $(M140_IMAGE_SRCS) $(M139_DIR_SRCS) $(M138_MFD_SRCS) $(M137_INDEX_SRCS) $(M126_DQ_READ_SRCS) $(M125_DQ_SRCS) $(M118_DB_SRCS) src/vms_boot_files11.c src/vms_bootio.c src/vms_qio_irp_prepare.c tests/test_f11_chain_extension.c -o build/f11chainextcheck
	./build/f11chainextcheck
.PHONY: f11chainboundscheck f11chainshortcheck
f11chainboundscheck: | build
	$(CC) -std=c11 -O2 -g -Wall -Wextra -Werror -fsanitize=undefined -fno-sanitize-recover=undefined -Isrc src/vms_f11_chain_lbn.c tests/test_f11_chain_bounds.c -o build/f11chainboundscheck
	./build/f11chainboundscheck
f11chainshortcheck: | build
	$(CC) -std=c11 -O2 -g -Wall -Wextra -Werror -fsanitize=undefined -fno-sanitize-recover=undefined -Isrc src/vms_f11_chain_lbn.c src/vms_f11_chain_read.c tests/test_f11_chain_shortio.c -o build/f11chainshortcheck
	./build/f11chainshortcheck
# M161-M168: eight source DQ_STARTIO/IOCIOPOST physical request functions.
# Device queue is private, with actual native ATA callback, not $QIO.
M168_IOC_SRCS=$(wildcard src/vms_dq_ioc_*.c)
M168_IOC_OBJS=$(patsubst src/%.c,build/%.o,$(M168_IOC_SRCS))
$(M168_IOC_OBJS): build/%.o: src/%.c src/vms_dq_ioc.h | build
	$(CC) $(CFLAGS) -c $< -o $@
.PHONY: dqprivateioccheck
dqprivateioccheck: $(M168_IOC_OBJS) | build
	$(CC) -std=c11 -O2 -g -Wall -Wextra -Werror -fsanitize=undefined -fno-sanitize-recover=undefined -Isrc -Itests $(M168_IOC_SRCS) $(wildcard tests/test_ioc_*_[0123].c) tests/test_ioc_main.c -o build/dqprivateioccheck
	./build/dqprivateioccheck
.PHONY: dqwriteioccheck
dqwriteioccheck: | build
	$(CC) -std=c11 -O2 -g -Wall -Wextra -Werror -fsanitize=undefined -fno-sanitize-recover=undefined -Isrc src/vms_dq_ioc_write.c tests/test_dq_ioc_write_host.c -o build/dqwriteioccheck
	./build/dqwriteioccheck
# M153-M160: eight source FIL$READVBN cursor functions on verified FH2
# extent chains, read-only privileged boot stage (not an ACP or channel).
M160_CURSOR_SRCS=$(wildcard src/vms_f11_cursor_*.c)
M160_CURSOR_OBJS=$(patsubst src/%.c,build/%.o,$(M160_CURSOR_SRCS))
$(M160_CURSOR_OBJS): build/%.o: src/%.c src/vms_f11_cursor.h | build
	$(CC) $(CFLAGS) -c $< -o $@
.PHONY: f11cursorcheck
f11cursorcheck: $(M160_CURSOR_OBJS) | build
	$(CC) -std=c11 -O2 -g -Wall -Wextra -Werror -fsanitize=undefined -fno-sanitize-recover=undefined -Isrc -Itests $(M160_CURSOR_SRCS) src/vms_f11_chain_lbn.c tests/test_cursor_*.c tests/test_f11_cursor_main.c -o build/f11cursorcheck
	./build/f11cursorcheck
# M177-M184: eight source FIL$READVBN -> EXE$INSIOQ read-only bridge routines.
# Private supervisor channel only: no ACP, $QIO, IRQ or live disk DDT.
M184_F11_IOC_SRCS=$(wildcard src/vms_f11_ioc_*.c)
M184_F11_IOC_OBJS=$(patsubst src/%.c,build/%.o,$(M184_F11_IOC_SRCS))
$(M184_F11_IOC_OBJS): build/%.o: src/%.c src/vms_f11_ioc.h | build
	$(CC) $(CFLAGS) -c $< -o $@
.PHONY: f11ioccheck
f11ioccheck: $(M184_F11_IOC_OBJS) | build
	$(CC) -std=c11 -O2 -g -Wall -Wextra -Werror -fsanitize=undefined -fno-sanitize-recover=undefined -Isrc -Itests $(M184_F11_IOC_SRCS) src/vms_f11_chain_lbn.c $(M168_IOC_SRCS) tests/test_f11_ioc_*_[0123].c tests/test_f11_ioc_main.c -o build/f11ioccheck
	./build/f11ioccheck
# M185-M192: bounded owner-scoped private Files-11 channel over IOC.
# Actual source FIL$READVBN bytes, no real process $ASSIGN or ACP yet.
M192_CHAN_SRCS=$(wildcard src/vms_f11_channel_*.c)
M192_CHAN_OBJS=$(patsubst src/%.c,build/%.o,$(M192_CHAN_SRCS))
$(M192_CHAN_OBJS): build/%.o: src/%.c src/vms_f11_channel.h src/vms_f11_channel_priv.h | build
	$(CC) $(CFLAGS) -c $< -o $@
.PHONY: f11channelcheck
f11channelcheck: $(M192_CHAN_OBJS) | build
	$(CC) -std=c11 -O2 -g -Wall -Wextra -Werror -fsanitize=undefined -fno-sanitize-recover=undefined -Isrc -Itests $(M192_CHAN_SRCS) $(M184_F11_IOC_SRCS) src/vms_f11_chain_lbn.c $(M168_IOC_SRCS) tests/test_f11_channel_*_[0123].c tests/test_f11_channel_main.c -o build/f11channelcheck
	./build/f11channelcheck
build/vms_v37_pcb.o: src/vms_v37_pcb.c src/vms_v37_pcb.h src/vms_v37_dyn_tags.h | build
	$(CC) $(CFLAGS) -c $< -o $@
build/sys.exe: build/vms_sys_executive_entry.o build/vms_v37_pcb.o build/vms_ini_syswcb.o build/vms_ini_pflvec.o build/vms_ini_mpw.o build/vms_ini_phv_swap.o build/vms_ini_procvec.o build/vms_ini_lockdb.o build/vms_sysboot_pktmem.o build/vms_ini_packet_queues.o build/vms_ini_log_templates.o build/vms_ini_syslog.o build/vms_ini_bootdev_scan.o build/vms_ini_dpt_templates.o build/vms_ini_dpt_mb.o build/vms_ini_dpt_nl.o build/vms_ini_dpt_op.o $(M118_DB_OBJS) $(M119_CON_OBJS) $(M120_IOC_OBJS) $(M121_QIO_OBJS) $(M218_SYSQIO_OBJS) $(M122_NL_DDT_OBJS) $(M123_IOPOST_OBJS) $(M125_DQ_OBJS) $(M126_DQ_READ_OBJS) $(M127_FIN_OBJS) $(M128_LOG_OBJS) $(M129_DEVLOG_OBJS) $(M130_DQDDT_OBJS) $(M131_RELOC_OBJS) $(M134_DDTRELOC_OBJS) $(M137_INDEX_OBJS) $(M138_MFD_OBJS) $(M139_DIR_OBJS) $(M140_IMAGE_OBJS) $(M144_AUX_OBJS) $(M146_RANGE_OBJS) $(M149_BOOTPATH_OBJS) $(M149_BOOTDIR_OBJS) $(M152_CHAIN_OBJS) $(M160_CURSOR_OBJS) $(M168_IOC_OBJS) $(M184_F11_IOC_OBJS) $(M192_CHAN_OBJS) $(M197_DQ_CRB_OBJS) build/vms_boot_files11.o build/vms_bootio.o build/executive-partial.o sys_native.ld
	$(LD) -m elf_i386 -T sys_native.ld -o $@ build/vms_sys_executive_entry.o build/vms_v37_pcb.o build/vms_ini_syswcb.o build/vms_ini_pflvec.o build/vms_ini_mpw.o build/vms_ini_phv_swap.o build/vms_ini_procvec.o build/vms_ini_lockdb.o build/vms_sysboot_pktmem.o build/vms_ini_packet_queues.o build/vms_ini_log_templates.o build/vms_ini_syslog.o build/vms_ini_bootdev_scan.o build/vms_ini_dpt_templates.o build/vms_ini_dpt_mb.o build/vms_ini_dpt_nl.o build/vms_ini_dpt_op.o $(M118_DB_OBJS) $(M119_CON_OBJS) $(M120_IOC_OBJS) $(M121_QIO_OBJS) $(M122_NL_DDT_OBJS) $(M123_IOPOST_OBJS) $(M125_DQ_OBJS) $(M126_DQ_READ_OBJS) $(M127_FIN_OBJS) $(M128_LOG_OBJS) $(M129_DEVLOG_OBJS) $(M130_DQDDT_OBJS) $(M131_RELOC_OBJS) $(M134_DDTRELOC_OBJS) $(M137_INDEX_OBJS) $(M138_MFD_OBJS) $(M139_DIR_OBJS) $(M140_IMAGE_OBJS) $(M144_AUX_OBJS) $(M146_RANGE_OBJS) $(M149_BOOTPATH_OBJS) $(M149_BOOTDIR_OBJS) $(M152_CHAIN_OBJS) $(M160_CURSOR_OBJS) $(M168_IOC_OBJS) $(M184_F11_IOC_OBJS) $(M192_CHAN_OBJS) $(M197_DQ_CRB_OBJS) build/vms_boot_files11.o build/vms_bootio.o build/executive-partial.o
build/vms_sysboot_elf.o: src/vms_sysboot_elf.c src/vms_sysboot_elf.h | build
	$(CC) $(CFLAGS) -c $< -o $@
build/install_sys_image: tools/install_sys_image.c src/vms_boot_files11.c src/vms_bootio.c src/vms_f11_home.c | build
	$(CC) -std=c11 -O2 -Wall -Wextra -Werror -Isrc $^ -o $@
.PHONY: sysnativecheck
sysnativecheck: portdisk.img build/sys.exe build/iniloa.exe build/sysloa386.exe | build
	$(CC) -std=c11 -O1 -g -fsanitize=address,undefined -fno-omit-frame-pointer -Wall -Wextra -Werror -Isrc src/vms_sysboot_elf.c src/vms_sysboot_read.c src/vms_sysboot_map.c src/vms_boot_files11.c src/vms_bootio.c tests/test_sysboot_elf_host.c -o build/sysnative_hosttest
	ASAN_OPTIONS=detect_leaks=1 ./build/sysnative_hosttest portdisk.img build/sys.exe build/iniloa.exe build/sysloa386.exe
.PHONY: qemu-sysnative
qemu-sysnative: vms37lab.ima portdisk.img
	python3 tools/qemu_sysboot_m97.py

# M89: original INILOA load order, with native IA-32 I/O-map counterpart.
build/vms_iniloa_iomap.o: src/vms_iniloa_iomap.c src/vms_iniloa_iomap.h | build
	$(CC) $(CFLAGS) -c $< -o $@
build/vms_iniloa_entry.o: src/vms_iniloa_entry.c src/vms_iniloa_entry.h src/vms_iniloa_iomap.h | build
	$(CC) $(CFLAGS) -c $< -o $@
build/iniloa.exe: build/vms_iniloa_iomap.o build/vms_iniloa_entry.o iniloa_native.ld
	$(LD) -m elf_i386 -T iniloa_native.ld -o $@ build/vms_iniloa_entry.o build/vms_iniloa_iomap.o
build/install_iniloa_image: tools/install_iniloa_image.c src/vms_boot_files11.c src/vms_bootio.c src/vms_f11_home.c | build
	$(CC) -std=c11 -O2 -Wall -Wextra -Werror -Isrc $^ -o $@
.PHONY: dqcrbcheck sysexecentrycheck qemu-sysboot-m197 qemu-sysboot-m198 qemu-sysboot-m199 qemu-sysboot-m201 qemu-sysboot-m202 qemu-sysboot-m203 qemu-sysboot-m204 qemu-sysboot-m205 qemu-sysboot-m206 qemu-sysboot-m207 qemu-sysboot-m209 qemu-sysboot-m210 qemu-sysboot-m211 qemu-sysboot-m212 qemu-sysboot-m213 qemu-sysboot-m214 qemu-sysboot-m216 qemu-sysboot-m217
dqcrbcheck: $(M197_DQ_CRB_OBJS) | build
	$(CC) -std=c11 -O1 -g -fsanitize=address,undefined -fno-omit-frame-pointer -Wall -Wextra -Werror -Isrc src/vms_dq_crb.c tests/test_dq_crb_host.c -o build/dqcrbcheck
	ASAN_OPTIONS=detect_leaks=1 ./build/dqcrbcheck
qemu-sysboot-m197: vms37lab.ima portdisk.img
	python3 tools/qemu_sysboot_m197.py
qemu-sysboot-m198: vms37lab.ima portdisk.img
	python3 tools/qemu_sysboot_m198.py
sysexecentrycheck: | build
	$(CC) -std=c11 -O1 -g -fsanitize=address,undefined -fno-omit-frame-pointer -Wall -Wextra -Werror -Isrc src/vms_sysloavec.c $(M118_DB_SRCS) $(M119_CON_SRCS) $(M120_IOC_SRCS) $(M121_QIO_SRCS) $(M122_NL_DDT_SRCS) $(M123_IOPOST_SRCS) $(M125_DQ_SRCS) $(M126_DQ_READ_SRCS) $(M127_FIN_SRCS) $(M128_LOG_SRCS) $(M129_DEVLOG_SRCS) $(M130_DQDDT_SRCS) $(M131_RELOC_SRCS) $(M134_DDTRELOC_SRCS) $(M137_INDEX_SRCS) $(M138_MFD_SRCS) $(M139_DIR_SRCS) $(M140_IMAGE_SRCS) $(M144_AUX_SRCS) $(M146_RANGE_SRCS) $(M149_BOOTPATH_SRCS) $(M149_BOOTDIR_SRCS) $(M152_CHAIN_SRCS) $(M160_CURSOR_SRCS) $(M168_IOC_SRCS) $(M184_F11_IOC_SRCS) $(M192_CHAN_SRCS) $(M197_DQ_CRB_SRCS) src/vms_boot_files11.c src/vms_bootio.c src/vms_sys_executive_entry.c src/vms_v37_pcb.c src/vms_ini_log_templates.c src/vms_ini_syslog.c src/vms_ini_bootdev_scan.c src/vms_ini_dpt_templates.c src/vms_ini_dpt_mb.c src/vms_ini_dpt_nl.c src/vms_ini_dpt_op.c src/vms_sys85.c src/vms_rtl85.c src/vms_rtl85_tables.c src/vms_ini_syswcb.c src/vms_ini_pflvec.c src/vms_ini_mpw.c src/vms_ini_phv_swap.c src/vms_ini_procvec.c src/vms_ini_lockdb.c src/vms_sysboot_pktmem.c src/vms_ini_packet_queues.c src/vms_exe_sched.c src/vms_exe_scbvector.c src/vms_exe_gettim.c src/vms_userptr.c tests/test_sys_exec_entry_host.c -o build/sysexecentrycheck
	ASAN_OPTIONS=detect_leaks=1 ./build/sysexecentrycheck
.PHONY: inicpbuscheck
inicpbuscheck: | build
	$(CC) -std=c11 -O1 -g -fsanitize=address,undefined -fno-omit-frame-pointer -Wall -Wextra -Werror -Isrc src/vms_ini_pc_bus.c src/vms_iniloa_iomap.c tests/test_ini_pc_bus_host.c -o build/inicpbuscheck
	ASAN_OPTIONS=detect_leaks=1 ./build/inicpbuscheck
.PHONY: iniloacheck
iniloacheck: portdisk.img build/iniloa.exe
	$(CC) -std=c11 -O1 -g -fsanitize=address,undefined -fno-omit-frame-pointer -Wall -Wextra -Werror -Isrc src/vms_iniloa_iomap.c tests/test_iniloa_iomap_host.c -o build/iniloa_iomap_hosttest
	ASAN_OPTIONS=detect_leaks=1 ./build/iniloa_iomap_hosttest

# M91: real loadable IA-32 SYSLOA386 code in original CPU-dependent order.
build/vms_sysloa386_cpu.o: src/vms_sysloa386_cpu.c src/vms_sysloa386.h | build
	$(CC) $(CFLAGS) -DPORT_SYSLOA386_GUEST -c $< -o $@
build/vms_sysloa386_boot.o: src/vms_sysloa386_boot.c src/vms_sysloa386.h | build
	$(CC) $(CFLAGS) -c $< -o $@
build/vms_sysloa386_entry.o: src/vms_sysloa386_entry.c src/vms_sysloa386.h | build
	$(CC) $(CFLAGS) -c $< -o $@
build/vms_sysloa386_vectors.o: src/vms_sysloa386_vectors.S | build
	$(CC) -m32 -c $< -o $@
build/sysloa386.exe: build/vms_sysloa386_vectors.o build/vms_sysloa386_entry.o build/vms_sysloa386_cpu.o build/vms_sysloa386_boot.o sysloa386_native.ld
	$(LD) -m elf_i386 -T sysloa386_native.ld -o $@ build/vms_sysloa386_vectors.o build/vms_sysloa386_entry.o build/vms_sysloa386_cpu.o build/vms_sysloa386_boot.o
build/install_sysloa386_image: tools/install_sysloa386_image.c src/vms_boot_files11.c src/vms_bootio.c src/vms_f11_home.c | build
	$(CC) -std=c11 -O2 -Wall -Wextra -Werror -Isrc $^ -o $@
.PHONY: sysloa386check
sysloa386check: | build
	$(CC) -std=c11 -O1 -g -fsanitize=address,undefined -fno-omit-frame-pointer -Wall -Wextra -Werror -Isrc src/vms_sysloa386_cpu.c src/vms_sysloa386_boot.c tests/test_sysloa386_host.c -o build/sysloa386check
	ASAN_OPTIONS=detect_leaks=1 ./build/sysloa386check
.PHONY: sysloa386imagecheck
sysloa386imagecheck: build/sysloa386.exe | build
	$(CC) -std=c11 -O2 -Wall -Wextra -Werror -Isrc src/vms_sysloavec.c tests/test_sysloa386_image_host.c -o build/sysloa386imagecheck
	./build/sysloa386imagecheck build/sysloa386.exe
# Recoverable checkpoints retain the exact COM1 bytes of every known run.
.PHONY: seriallogcheck seriallogtest checkpointzip
seriallogcheck:
	python3 tools/checkpoint_serial_logs.py verify
seriallogtest:
	python3 tests/test_checkpoint_serial_logs.py
checkpointzip: seriallogtest seriallogcheck
	python3 tools/checkpoint_serial_logs.py bundle --output ../$(notdir $(CURDIR))-current.zip

# M93: original BOOTS SYSBOOT packet carve and INIT.MAR lookaside queue headers.
.PHONY: pktlayoutcheck packetqueuecheck
pktlayoutcheck: | build
	$(CC) -std=c11 -O1 -g -fsanitize=address,undefined -fno-omit-frame-pointer -Wall -Wextra -Werror -Isrc src/vms_sysboot_pktmem.c tests/test_sysboot_pktmem_host.c -o build/pktlayoutcheck
	ASAN_OPTIONS=detect_leaks=1 ./build/pktlayoutcheck
packetqueuecheck: | build
	$(CC) -std=c11 -O1 -g -fsanitize=address,undefined -fno-omit-frame-pointer -Wall -Wextra -Werror -Isrc src/vms_ini_packet_queues.c tests/test_ini_packet_queues_host.c -o build/packetqueuecheck
	ASAN_OPTIONS=detect_leaks=1 ./build/packetqueuecheck

# M94: preserve INIT.MAR realtime-SFT and lock-data-memory record behavior.
.PHONY: inilockdbcheck
inilockdbcheck: | build
	$(CC) -std=c11 -O1 -g -fsanitize=address,undefined -fno-omit-frame-pointer -Wall -Wextra -Werror -Isrc src/vms_ini_lockdb.c tests/test_ini_lockdb_host.c -o build/inilockdbcheck
	ASAN_OPTIONS=detect_leaks=1 ./build/inilockdbcheck

# M95: original INIT.MAR SYSTEM/NULL PCB vector and sequence array layout.
.PHONY: iniprocveccheck
iniprocveccheck: | build
	$(CC) -std=c11 -O1 -g -fsanitize=address,undefined -fno-omit-frame-pointer -Wall -Wextra -Werror -Isrc src/vms_ini_procvec.c tests/test_ini_procvec_host.c -o build/iniprocveccheck
	ASAN_OPTIONS=detect_leaks=1 ./build/iniprocveccheck

# M96: original INIT.MAR INI_PHV and INI_SWAP persistent DYN records.
.PHONY: iniphvswapcheck
iniphvswapcheck: | build
	$(CC) -std=c11 -O1 -g -fsanitize=address,undefined -fno-omit-frame-pointer -Wall -Wextra -Werror -Isrc src/vms_ini_phv_swap.c tests/test_ini_phv_swap_host.c -o build/iniphvswapcheck
	ASAN_OPTIONS=detect_leaks=1 ./build/iniphvswapcheck

# M97: INIT.MAR modified-page writer PTE/PHV index allocation.
.PHONY: inimpwcheck
inimpwcheck: | build
	$(CC) -std=c11 -O1 -g -fsanitize=address,undefined -fno-omit-frame-pointer -Wall -Wextra -Werror -Isrc src/vms_ini_mpw.c tests/test_ini_mpw_host.c -o build/inimpwcheck
	ASAN_OPTIONS=detect_leaks=1 ./build/inimpwcheck

# M98: source-derived PFL vectors remain host-only; the source-defined
# INI_INTSTKLM interrupt-stack depth is executed by guest SYS.EXE from M101.
build/vms_ini_pflvec.o: src/vms_ini_pflvec.c src/vms_ini_pflvec.h | build
	$(CC) $(CFLAGS) -c $< -o $@
.PHONY: inipflveccheck
inipflveccheck: build/vms_ini_pflvec.o | build
	$(CC) -std=c11 -O1 -g -fsanitize=address,undefined -fno-omit-frame-pointer -Wall -Wextra -Werror -Isrc src/vms_ini_pflvec.c tests/test_ini_pflvec_host.c -o build/inipflveccheck
	ASAN_OPTIONS=detect_leaks=1 ./build/inipflveccheck

# M99: INIT.MAR boot-control-block normalized SYS.EXE map to real WCB
# retrieval pointers. Host-only pending linked SYSWCB template/boot CB.
build/vms_ini_syswcb.o: src/vms_ini_syswcb.c src/vms_ini_syswcb.h | build
	$(CC) $(CFLAGS) -c $< -o $@
.PHONY: inisyswcbcheck
inisyswcbcheck: build/vms_ini_syswcb.o | build
	$(CC) -std=c11 -O1 -g -fsanitize=address,undefined -fno-omit-frame-pointer -Wall -Wextra -Werror -Isrc src/vms_ini_syswcb.c tests/test_ini_syswcb_host.c -o build/inisyswcbcheck
	ASAN_OPTIONS=detect_leaks=1 ./build/inisyswcbcheck

.PHONY: inisyswcbdiskcheck
inisyswcbdiskcheck: portdisk.img | build
	$(CC) -std=c11 -O1 -g -fsanitize=address,undefined -fno-omit-frame-pointer -Wall -Wextra -Werror -Isrc src/vms_ini_syswcb.c src/vms_boot_files11.c src/vms_bootio.c src/vms_f11_home.c tests/test_ini_syswcb_disk.c -o build/inisyswcbdiskcheck
	ASAN_OPTIONS=detect_leaks=1 ./build/inisyswcbdiskcheck portdisk.img

# M100: SYS/SRC/INIT.MAR INI_LOG two nonpaged logical records.
build/vms_ini_syslog.o: src/vms_ini_syslog.c src/vms_ini_syslog.h | build
	$(CC) $(CFLAGS) -c $< -o $@
.PHONY: inisyslogcheck
inisyslogcheck: build/vms_ini_syslog.o | build
	$(CC) -std=c11 -O1 -g -fsanitize=address,undefined -fno-omit-frame-pointer -Wall -Wextra -Werror -Isrc src/vms_ini_syslog.c tests/test_ini_syslog_host.c -o build/inisyslogcheck
	ASAN_OPTIONS=detect_leaks=1 ./build/inisyslogcheck

# M102 partial original INI driver-prologue list: no fake MB/NL/OP drivers.
build/vms_ini_dptlist.o: src/vms_ini_dptlist.c src/vms_ini_dptlist.h | build
	$(CC) $(CFLAGS) -c $< -o $@
.PHONY: inidptlistcheck
inidptlistcheck: build/vms_ini_dptlist.o | build
	$(CC) -std=c11 -O1 -g -fsanitize=address,undefined -fno-omit-frame-pointer -Wall -Wextra -Werror -Isrc src/vms_ini_dptlist.c tests/test_ini_dptlist_host.c -o build/inidptlistcheck
	ASAN_OPTIONS=detect_leaks=1 ./build/inidptlistcheck

# M103: source-defined TTDRIVER class-vector relocation; unbound driver.
build/vms_ini_ttydrv.o: src/vms_ini_ttydrv.c src/vms_ini_ttydrv.h | build
	$(CC) $(CFLAGS) -c $< -o $@
.PHONY: inittydrvcheck
inittydrvcheck: build/vms_ini_ttydrv.o | build
	$(CC) -std=c11 -O1 -g -fsanitize=address,undefined -fno-omit-frame-pointer -Wall -Wextra -Werror -Isrc src/vms_ini_ttydrv.c tests/test_ini_ttydrv_host.c -o build/inittydrvcheck
	ASAN_OPTIONS=detect_leaks=1 ./build/inittydrvcheck

.PHONY: inittybindcheck
inittybindcheck: build/vms_ini_ttydrv.o | build
	$(CC) -std=c11 -O1 -g -fsanitize=address,undefined -fno-omit-frame-pointer -Wall -Wextra -Werror -Isrc src/vms_ini_ttydrv.c tests/test_ini_ttybind_host.c -o build/inittybindcheck
	ASAN_OPTIONS=detect_leaks=1 ./build/inittybindcheck

# M105: source-ordered SYSBOOT BOO$L_SYS_MAP on genuine Files-11 SYS.EXE.
.PHONY: sysnormalizedmapcheck
sysnormalizedmapcheck: portdisk.img | build
	$(CC) -std=c11 -O1 -g -fsanitize=address,undefined -fno-omit-frame-pointer -Wall -Wextra -Werror -Isrc src/vms_sysboot_map.c src/vms_boot_files11.c src/vms_bootio.c tests/test_sysboot_normalized_map.c -o build/sysnormalizedmapcheck
	ASAN_OPTIONS=detect_leaks=1 ./build/sysnormalizedmapcheck portdisk.img

# M107: every source-specific translation has its own native IA-32 object.
M107_NAMES := mthiidnnt mthiignnt mthiihnnt mthjidnnt mthjignnt mthjihnnt mthinint mthjnint mthiisign mthjisign mthimax0 mthimin0 mthjmax0 mthjmin0 mthsign mthdsign mthgsign mthhsign mthmax1 mthmin1 mthdmax1 mthdmin1 mthgmax1 mthgmin1 mthhmax1 mthhmin1 mthconjg mthdconjg mthgconjg mthsgn comr50wd comrad50 comirad50 comr50asc
M107_OBJECTS := $(addprefix build/m107/,$(addsuffix .o,$(M107_NAMES)))
build/vms_rtl_m107.o: src/vms_rtl_m107.c src/vms_rtl_m107.h | build
	$(CC) $(EXEFLAGS) -c $< -o $@
build/m107/%.o: src/native_rtl_m107/%.c src/vms_rtl_m107.h | build
	mkdir -p build/m107
	$(CC) $(EXEFLAGS) -c $< -o $@
build/rtl-m107-partial.o: build/vms_rtl_m107.o $(M107_OBJECTS)
	$(LD) -m elf_i386 -r -o $@ $^
.PHONY: rtl107check
rtl107check: | build
	$(CC) -std=c11 -O2 -Wall -Wextra -Werror -I src src/vms_rtl_m107.c src/native_rtl_m107/*.c tests/test_rtl_m107.c -o build/rtl107check
	./build/rtl107check
all: build/rtl-m107-partial.o

# M108: 38 distinct original RTL modules translated to IA-32 relocatable units.
M108_MODULE_SRCS=$(wildcard src/native_rtl_m108/*.c)
M108_MODULE_OBJS=$(patsubst src/native_rtl_m108/%.c,build/m108/%.o,$(M108_MODULE_SRCS))
build/vms_rtl_m108.o: src/vms_rtl_m108.c src/vms_rtl_m108.h src/vms_rtl_m107.h | build
	$(CC) $(CFLAGS) -c $< -o $@
build/m108/%.o: src/native_rtl_m108/%.c src/vms_rtl_m108.h | build
	mkdir -p build/m108
	$(CC) $(CFLAGS) -c $< -o $@
build/rtl-m108-partial.o: build/vms_rtl_m108.o $(M108_MODULE_OBJS)
	$(LD) -m elf_i386 -r -o $@ $^
.PHONY: rtl108check
rtl108check: build/rtl-m108-partial.o
	$(CC) -std=c11 -O2 -g -fsanitize=undefined -fno-sanitize-recover=undefined -Wall -Wextra -Werror -Isrc src/vms_rtl_m107.c src/vms_rtl_m108.c $(M108_MODULE_SRCS) tests/test_rtl_m108.c -o build/rtl108check
	./build/rtl108check
all: build/rtl-m108-partial.o

# M109: 36 pinned DEC RTL math source modules, native self-contained x87 IA-32 subsets.
M109_MODULE_SRCS=$(wildcard src/native_rtl_m109/*.c)
M109_MODULE_OBJS=$(patsubst src/native_rtl_m109/%.c,build/m109/%.o,$(M109_MODULE_SRCS))
build/vms_rtl_m109_x87.o: src/vms_rtl_m109_x87.c src/vms_rtl_m109_x87.h | build
	$(CC) $(CFLAGS) -fplt -c $< -o $@
build/vms_rtl_m109.o: src/vms_rtl_m109.c src/vms_rtl_m109.h src/vms_rtl_m109_x87.h | build
	$(CC) $(CFLAGS) -fplt -c $< -o $@
build/m109/%.o: src/native_rtl_m109/%.c src/vms_rtl_m109.h | build
	mkdir -p build/m109
	$(CC) $(CFLAGS) -fplt -c $< -o $@
build/rtl-m109-partial.o: build/vms_rtl_m109.o build/vms_rtl_m109_x87.o $(M109_MODULE_OBJS)
	ld -m elf_i386 -r -o $@ $^
.PHONY: rtl109check
rtl109check: build/rtl-m109-partial.o
	$(CC) -std=c11 -O2 -g -fsanitize=undefined -fno-sanitize-recover=undefined -Wall -Wextra -Werror -Isrc src/vms_rtl_m107.c src/vms_rtl_m108.c src/vms_rtl_m109.c src/vms_rtl_m109_x87.c $(M109_MODULE_SRCS) tests/test_rtl_m109.c -o build/rtl109check
	./build/rtl109check
	python3 tools/gen_source_coverage.py
	python3 tests/test_rtl_m109_integrity.py
all: build/rtl-m109-partial.o

# M110: strict original line inventory checker and maintained CHS-stage test.
.PHONY: sourcerangecheck m21bootcheck
sourcerangecheck:
	python3 tools/gen_source_coverage.py
	python3 tools/validate_source_ranges.py
	python3 tests/test_coverage_ranges.py
m21bootcheck: build/boot.bin build/kernel.bin vms37lab.ima
	python3 tests/test_m21_boot.py

# M110: 34 original RTL module slices alongside two SYS executive mappings.
M110_MODULE_SRCS=$(wildcard src/native_rtl_m110/*.c)
M110_MODULE_OBJS=$(patsubst src/native_rtl_m110/%.c,build/m110/%.o,$(M110_MODULE_SRCS))
build/vms_rtl_m110.o: src/vms_rtl_m110.c src/vms_rtl_m110.h src/vms_rtl_m109.h | build
	$(CC) $(EXEFLAGS) -c $< -o $@
build/m110/%.o: src/native_rtl_m110/%.c src/vms_rtl_m110.h | build
	mkdir -p build/m110
	$(CC) $(EXEFLAGS) -c $< -o $@
build/rtl-m110-partial.o: build/vms_rtl_m110.o build/vms_rtl_m109.o build/vms_rtl_m109_x87.o $(M110_MODULE_OBJS)
	$(LD) -m elf_i386 -r -o $@ $^
.PHONY: rtl110check
rtl110check: build/rtl-m110-partial.o
	$(CC) -std=c11 -O2 -g -Wall -Wextra -Werror -fsanitize=undefined -fno-sanitize-recover=undefined -Isrc src/vms_rtl_m109.c src/vms_rtl_m109_x87.c src/vms_rtl_m110.c $(M110_MODULE_SRCS) tests/test_rtl_m110.c -o build/rtl110check
	./build/rtl110check
	python3 tests/test_rtl_m110_integrity.py
all: build/rtl-m110-partial.o

# M111: 40 pinned VMS 3.7 RTL source modules compiled to native IA-32.
M111_MODULE_SRCS=$(wildcard src/native_rtl_m111/*.c)
M111_MODULE_OBJS=$(patsubst src/native_rtl_m111/%.c,build/m111/%.o,$(M111_MODULE_SRCS))
build/vms_rtl_m111.o: src/vms_rtl_m111.c src/vms_rtl_m111.h src/vms_rtl_m109_x87.h | build
	$(CC) $(EXEFLAGS) -c $< -o $@
build/m111/%.o: src/native_rtl_m111/%.c src/vms_rtl_m111.h | build
	mkdir -p build/m111
	$(CC) $(EXEFLAGS) -c $< -o $@
build/rtl-m111-partial.o: build/vms_rtl_m111.o build/vms_rtl_m109.o build/vms_rtl_m109_x87.o $(M111_MODULE_OBJS)
	$(LD) -m elf_i386 -r -o $@ $^
.PHONY: rtl111check
rtl111check: build/rtl-m111-partial.o
	$(CC) -std=c11 -O2 -g -Wall -Wextra -Werror -fsanitize=undefined -fno-sanitize-recover=undefined -Isrc src/vms_rtl_m111.c src/vms_rtl_m109.c src/vms_rtl_m109_x87.c $(M111_MODULE_SRCS) tests/test_rtl_m111.c -o build/rtl111check
	./build/rtl111check
	python3 tools/gen_source_coverage.py
	python3 tests/test_rtl_m111_integrity.py
all: build/rtl-m111-partial.o

# M112: 30 exact FORTRAN statement entry dispatchers + 10 original FORTRAN RTL sources.
M112_MODULE_SRCS=$(wildcard src/native_rtl_m112/*.c)
M112_MODULE_OBJS=$(patsubst src/native_rtl_m112/%.c,build/m112/%.o,$(M112_MODULE_SRCS))
build/vms_for_m112.o: src/vms_for_m112.c src/vms_for_m112.h | build
	$(CC) $(EXEFLAGS) -c $< -o $@
build/m112/%.o: src/native_rtl_m112/%.c src/vms_for_m112.h | build
	mkdir -p build/m112
	$(CC) $(EXEFLAGS) -c $< -o $@
build/rtl-m112-partial.o: build/vms_for_m112.o $(M112_MODULE_OBJS)
	$(LD) -m elf_i386 -r -o $@ $^
.PHONY: rtl112check
rtl112check: build/rtl-m112-partial.o
	$(CC) -std=c11 -O2 -g -Wall -Wextra -Werror -fsanitize=undefined -fno-sanitize-recover=undefined -Isrc src/vms_for_m112.c $(M112_MODULE_SRCS) tests/test_rtl_m112.c -o build/rtl112check
	./build/rtl112check
	python3 tools/gen_source_coverage.py
	python3 tests/test_rtl_m112_integrity.py
all: build/rtl-m112-partial.o

# M113: 40 original source-specific COBOL/BASIC/LIB RTL slices; host-only.
M113_MODULE_SRCS=$(wildcard src/native_rtl_m113/*.c)
M113_MODULE_OBJS=$(patsubst src/native_rtl_m113/%.c,build/m113/%.o,$(M113_MODULE_SRCS))
build/vms_rtl_m113.o: src/vms_rtl_m113.c src/vms_rtl_m113.h src/vms_rtl_m109.h | build
	$(CC) $(EXEFLAGS) -c $< -o $@
build/vms_rtl_m113_tables.o: src/vms_rtl_m113_tables.c src/vms_rtl_m113.h | build
	$(CC) $(EXEFLAGS) -c $< -o $@
build/m113/%.o: src/native_rtl_m113/%.c src/vms_rtl_m113.h | build
	mkdir -p build/m113
	$(CC) $(EXEFLAGS) -c $< -o $@
build/rtl-m113-partial.o: build/vms_rtl_m113.o build/vms_rtl_m113_tables.o build/vms_rtl_m109.o build/vms_rtl_m109_x87.o $(M113_MODULE_OBJS)
	$(LD) -m elf_i386 -r -o $@ $^
.PHONY: rtl113check
rtl113check: build/rtl-m113-partial.o
	$(CC) -std=c11 -O2 -g -Wall -Wextra -Werror -fsanitize=undefined -fno-sanitize-recover=undefined -Isrc src/vms_rtl_m109.c src/vms_rtl_m109_x87.c src/vms_rtl_m113.c src/vms_rtl_m113_tables.c $(M113_MODULE_SRCS) tests/test_rtl_m113.c -o build/rtl113check
	./build/rtl113check
	python3 tools/gen_source_coverage.py
	python3 tests/test_rtl_m113_integrity.py
all: build/rtl-m113-partial.o

# M114: true source-size INIT.MAR logical-name templates in resident SYS.EXE.
build/vms_ini_log_templates.o: src/vms_ini_log_templates.c src/vms_ini_log_templates.h src/vms_v37_dyn_tags.h | build
	$(CC) $(CFLAGS) -c $< -o $@
.PHONY: inilogtemplatecheck
inilogtemplatecheck: build/vms_ini_log_templates.o build/vms_ini_syslog.o | build
	$(CC) -std=c11 -O2 -Wall -Wextra -Werror -Isrc src/vms_ini_log_templates.c src/vms_ini_syslog.c tests/test_ini_log_templates_host.c -o build/inilogtemplatecheck
	./build/inilogtemplatecheck

# M115: original NLDRIVER FDT runs from loaded SYS.EXE, plus INIT.MAR
# registered native ATA boot-adapter source-order scan, not a live DDB.
build/vms_ini_bootdev_scan.o: src/vms_ini_bootdev_scan.c src/vms_ini_bootdev_scan.h src/vms_iniloa_iomap.h | build
	$(CC) $(EXEFLAGS) -c $< -o $@
.PHONY: inibootdevscancheck
inibootdevscancheck: build/vms_ini_bootdev_scan.o | build
	$(CC) -std=c11 -O2 -g -Wall -Wextra -Werror -fsanitize=undefined -fno-sanitize-recover=undefined -Isrc src/vms_ini_bootdev_scan.c tests/test_ini_bootdev_scan_host.c -o build/inibootdevscancheck
	./build/inibootdevscancheck

# M116: DYN typed record values taken from VMS 3.7 SYSDEF.MDL,
# with LOG$B_TYPE=11 in the guest. No live logical-name lookup yet.
.PHONY: v37dyntagcheck
v37dyntagcheck: | build
	$(CC) -std=c11 -O2 -Wall -Wextra -Werror -Isrc tests/test_vms_v37_dyn_tags_host.c -o build/v37dyntagcheck
	./build/v37dyntagcheck

# M117: three source-specific DPT header records; real MB/OP driver images
# and IOC$GL_DPTLIST are still missing. DPT$W_SIZE stays zero by design.
M117_DPT_SRCS=src/vms_ini_dpt_templates.c src/vms_ini_dpt_mb.c src/vms_ini_dpt_nl.c src/vms_ini_dpt_op.c
build/vms_ini_dpt_templates.o: src/vms_ini_dpt_templates.c src/vms_ini_dpt_templates.h src/vms_v37_dyn_tags.h | build
	$(CC) $(CFLAGS) -c $< -o $@
build/vms_ini_dpt_mb.o: src/vms_ini_dpt_mb.c src/vms_ini_dpt_templates.h src/vms_ini_devicedb.h src/vms_con_port.h | build
	$(CC) $(CFLAGS) -c $< -o $@
build/vms_ini_dpt_nl.o: src/vms_ini_dpt_nl.c src/vms_ini_dpt_templates.h | build
	$(CC) $(CFLAGS) -c $< -o $@
build/vms_ini_dpt_op.o: src/vms_ini_dpt_op.c src/vms_ini_dpt_templates.h | build
	$(CC) $(CFLAGS) -c $< -o $@
.PHONY: inidpttemplatecheck
inidpttemplatecheck: build/vms_ini_dpt_templates.o build/vms_ini_dpt_mb.o build/vms_ini_dpt_nl.o build/vms_ini_dpt_op.o | build
	$(CC) -std=c11 -O2 -g -Wall -Wextra -Werror -fsanitize=undefined -fno-sanitize-recover=undefined -Isrc $(M117_DPT_SRCS) tests/test_ini_dpt_templates_host.c -o build/inidpttemplatecheck
	./build/inidpttemplatecheck

# M120: original IOSUBPAGD read-only IOC$SEARCHDEV physical device walk.
# A prelinked DDB/UCB match remains UNBOUND until real $ASSIGN/$QIO and DDT.
build/vms_ioc_%.o: src/vms_ioc_%.c src/vms_ioc_phys.h src/vms_ini_devicedb.h | build
	$(CC) $(CFLAGS) -c $< -o $@
.PHONY: iocphyscheck
iocphyscheck: $(M120_IOC_OBJS) | build
	$(CC) -std=c11 -O2 -g -Wall -Wextra -Werror -fsanitize=undefined -fno-sanitize-recover=undefined -Isrc $(M120_IOC_SRCS) $(M118_DB_SRCS) tests/test_ioc_phys_host.c -o build/iocphyscheck
	./build/iocphyscheck

# M121: source SYSQIOREQ.MAR EXE$FINISHIOC to IOC post queue, used
# by NLDRIVER.MAR; this is not a user $QIO/AST delivery interface.
$(M121_QIO_OBJS): build/%.o: src/%.c src/vms_qio_post.h src/vms_qio_irp_pool.h src/vms_ini_packet_queues.h src/vms_sysqio_args.h src/vms_v37_pcb.h src/vms_dq_fdt.h | build
	$(CC) $(CFLAGS) -c $< -o $@
.PHONY: qiopostcheck
qiopostcheck: $(M121_QIO_OBJS) | build
	$(CC) -std=c11 -O2 -g -Wall -Wextra -Werror -fsanitize=undefined -fno-sanitize-recover=undefined -Isrc $(M121_QIO_SRCS) $(M118_DB_SRCS) src/vms_sys85.c src/vms_rtl85.c src/vms_rtl85_tables.c tests/test_qio_post_host.c -o build/qiopostcheck
	./build/qiopostcheck

# M234: SYSQIO's source MEMORYALC IRP lookaside allocate/return operations.
.PHONY: irpalloccheck
irpalloccheck: | build
	$(CC) -std=c11 -O1 -g -fsanitize=address,undefined -fno-omit-frame-pointer -Wall -Wextra -Werror -Isrc src/vms_ini_packet_queues.c src/vms_qio_irp_pool.c tests/test_irp_pool_host.c -o build/irpalloccheck
	ASAN_OPTIONS=detect_leaks=1 ./build/irpalloccheck

# M122: source SYSMAR DDTAB/FUNCTAB, NLDRIVER and DEVICEDAT NL$DDT.
$(M122_NL_DDT_OBJS): build/%.o: src/%.c src/vms_nl_ddt.h src/vms_qio_post.h | build
	$(CC) $(CFLAGS) -c $< -o $@
.PHONY: nlddtcheck
nlddtcheck: $(M122_NL_DDT_OBJS) | build
	$(CC) -std=c11 -O2 -g -Wall -Wextra -Werror -fsanitize=undefined -fno-sanitize-recover=undefined -Isrc $(M122_NL_DDT_SRCS) $(M121_QIO_SRCS) $(M120_IOC_SRCS) $(M118_DB_SRCS) src/vms_sys85.c src/vms_rtl85.c src/vms_rtl85_tables.c tests/test_nl_ddt_host.c -o build/nlddtcheck
	./build/nlddtcheck

# M123: bounded source IOCIOPOST kernel end-action only; PCB/KAST absent.
$(M123_IOPOST_OBJS): build/%.o: src/%.c src/vms_iopost_end.h src/vms_qio_post.h | build
	$(CC) $(CFLAGS) -c $< -o $@
.PHONY: iopostendcheck
iopostendcheck: $(M123_IOPOST_OBJS) | build
	$(CC) -std=c11 -O2 -g -Wall -Wextra -Werror -fsanitize=undefined -fno-sanitize-recover=undefined -Isrc $(M123_IOPOST_SRCS) $(M122_NL_DDT_SRCS) $(M121_QIO_SRCS) $(M120_IOC_SRCS) $(M118_DB_SRCS) src/vms_sys85.c src/vms_rtl85.c src/vms_rtl85_tables.c tests/test_iopost_end_host.c -o build/iopostendcheck
	./build/iopostendcheck

# M125: source 12 DQDRIVER.MAR FDT function masks (guest staged only;
# original RB730/ACP/CRB and native ATA controller bindings remain absent).
build/vms_dq_fdt_%.o: src/vms_dq_fdt_%.c src/vms_dq_fdt.h | build
	$(CC) $(CFLAGS) -c $< -o $@
.PHONY: dqfdtcheck
dqfdtcheck: $(M125_DQ_OBJS) | build
	$(CC) -std=c11 -O2 -g -Wall -Wextra -Werror -fsanitize=undefined -fno-sanitize-recover=undefined -Isrc $(M125_DQ_SRCS) tests/test_dq_fdt_host.c -o build/dqfdtcheck
	./build/dqfdtcheck

# M126: one genuine privileged DQ READPBLK at the SYSBOOT/INIT boundary.
$(M126_DQ_READ_OBJS): build/%.o: src/%.c src/vms_dq_bootread.h src/vms_qio_post.h | build
	$(CC) $(CFLAGS) -c $< -o $@
.PHONY: dqbootreadcheck
dqbootreadcheck: $(M126_DQ_READ_OBJS) portdisk.img | build
	$(CC) -std=c11 -O2 -g -Wall -Wextra -Werror -fsanitize=address,undefined -fno-sanitize-recover=undefined -Isrc $(M126_DQ_READ_SRCS) $(M125_DQ_SRCS) $(M118_DB_SRCS) src/vms_dq_ioc_write.c src/vms_qio_irp_prepare.c tests/test_dq_bootread_host.c -o build/dqbootreadcheck
	ASAN_OPTIONS=detect_leaks=1 ./build/dqbootreadcheck

# M127: INIT.MAR FIN_DATABASE source DDB names and boot logical equivalences.
$(M127_FIN_OBJS): build/%.o: src/%.c src/vms_ini_fin_database.h | build
	$(CC) $(CFLAGS) -c $< -o $@
.PHONY: inifindatabasecheck
inifindatabasecheck: $(M127_FIN_OBJS) | build
	$(CC) -std=c11 -O2 -g -Wall -Wextra -Werror -fsanitize=address,undefined -fno-sanitize-recover=undefined -Isrc $(M127_FIN_SRCS) $(M118_DB_SRCS) src/vms_ini_log_templates.c src/vms_ini_syslog.c tests/test_ini_fin_database_host.c -o build/inifindatabasecheck
	ASAN_OPTIONS=detect_leaks=1 ./build/inifindatabasecheck

# M128: LOGNAMSUB.MAR LOG$HASH and bounded read-only SYSTEM translation.
$(M128_LOG_OBJS): build/%.o: src/%.c src/vms_log_system.h | build
	$(CC) $(CFLAGS) -c $< -o $@
.PHONY: logsystemcheck
logsystemcheck: $(M128_LOG_OBJS) | build
	$(CC) -std=c11 -O2 -g -Wall -Wextra -Werror -fsanitize=address,undefined -fno-sanitize-recover=undefined -Isrc $(M128_LOG_SRCS) $(M127_FIN_SRCS) $(M118_DB_SRCS) src/vms_ini_log_templates.c src/vms_ini_syslog.c tests/test_log_system_host.c -o build/logsystemcheck
	ASAN_OPTIONS=detect_leaks=1 ./build/logsystemcheck

# M129: IOSUBPAGD source IOC$SEARCHDEV LOG$TRANSLATE 10-iteration loop,
# backed solely by resident bootstrap SYSTEM log and prelinked DDBs.
$(M129_DEVLOG_OBJS): build/%.o: src/%.c src/vms_devlog_search.h | build
	$(CC) $(CFLAGS) -c $< -o $@
.PHONY: devlogcheck
devlogcheck: $(M129_DEVLOG_OBJS) | build
	$(CC) -std=c11 -O2 -g -Wall -Wextra -Werror -fsanitize=address,undefined -fno-omit-frame-pointer -Isrc $(M129_DEVLOG_SRCS) $(M128_LOG_SRCS) $(M127_FIN_SRCS) $(M120_IOC_SRCS) $(M118_DB_SRCS) src/vms_ini_log_templates.c src/vms_ini_syslog.c tests/test_devlog_search_host.c -o build/devlogcheck
	ASAN_OPTIONS=detect_leaks=1 ./build/devlogcheck

# M130-M207: source DQ DDT/FDT activation, partial DQA0 publication and
# IOC-selected STARTIO. Full RB730 IOC/error paths, REGDUMP, MNTVER and ACP
# remain unported.
$(M130_DQDDT_OBJS): build/%.o: src/%.c src/vms_dqddt.h src/vms_dq_bootread.h src/vms_ini_devicedb.h src/vms_dq_crb.h src/vms_qio_post.h | build
	$(CC) $(CFLAGS) -c $< -o $@
.PHONY: dqddtcheck
dqddtcheck: $(M130_DQDDT_OBJS) | build
	$(CC) -std=c11 -O2 -g -Wall -Wextra -Werror -fsanitize=address,undefined -fno-sanitize-recover=undefined -Isrc $(M130_DQDDT_SRCS) $(M134_DDTRELOC_SRCS) $(M197_DQ_CRB_SRCS) $(M126_DQ_READ_SRCS) $(M125_DQ_SRCS) $(M120_IOC_SRCS) src/vms_dq_ioc_write.c src/vms_qio_irp_prepare.c src/vms_ini_devicedb.c $(filter-out src/vms_ini_devicedb.c,$(M118_DB_SRCS)) tests/test_dqddt_host.c -o build/dqddtcheck
	./build/dqddtcheck

# M131: original RELOCDRV IOC$INITDRV DPT STORE operations, no DDT bind.
$(M131_RELOC_OBJS): build/%.o: src/%.c src/vms_relocdrv.h | build
	$(CC) $(CFLAGS) -c $< -o $@
.PHONY: relocdrvcheck
relocdrvcheck: $(M131_RELOC_OBJS) | build
	$(CC) -std=c11 -O2 -g -Wall -Wextra -Werror -fsanitize=undefined -fno-sanitize-recover=undefined -Isrc $(M131_RELOC_SRCS) $(M130_DQDDT_SRCS) $(M126_DQ_READ_SRCS) $(M125_DQ_SRCS) src/vms_dq_ioc_write.c $(M118_DB_SRCS) tests/test_relocdrv_host.c -o build/relocdrvcheck
	./build/relocdrvcheck

# M134: original IOC$RELOC_DDT private-table relocation, never active DQA0.
$(M134_DDTRELOC_OBJS): build/%.o: src/%.c src/vms_ddtreloc.h | build
	$(CC) $(CFLAGS) -c $< -o $@
.PHONY: ddtreloccheck
ddtreloccheck: $(M134_DDTRELOC_OBJS) | build
	$(CC) -std=c11 -O2 -g -Wall -Wextra -Werror -fsanitize=undefined -fno-sanitize-recover=undefined -Isrc $(M134_DDTRELOC_SRCS) tests/test_ddtreloc_host.c -o build/ddtreloccheck
	./build/ddtreloccheck
.PHONY: ddtreloccasescheck
ddtreloccasescheck: | build
	$(CC) -std=c11 -O2 -g -Wall -Wextra -Werror -fsanitize=address,undefined -fno-sanitize-recover=undefined -Isrc -Itests $(M134_DDTRELOC_SRCS) tests/test_ddtreloc_case_*.c tests/test_ddtreloc_cases_main.c -o build/ddtreloccasescheck
	./build/ddtreloccasescheck

# M136: 34 original DQ physical READPBLK LBN/IOST cases on private DDB/UCB.
.PHONY: dqphysicalcheck
dqphysicalcheck: portdisk.img | build
	$(CC) -std=c11 -O2 -g -Wall -Wextra -Werror -fsanitize=undefined -fno-sanitize-recover=undefined -Isrc -Itests $(M126_DQ_READ_SRCS) $(M125_DQ_SRCS) $(M118_DB_SRCS) src/vms_dq_ioc_write.c src/vms_qio_irp_prepare.c tests/test_dq_physical_case_*.c tests/test_dq_physical_cases_main.c -o build/dqphysicalcheck
	./build/dqphysicalcheck

# M145: FIL$FINDFILID scans VBNs across nonadjacent FH2 file extents.
.PHONY: dqdirblockscheck
dqdirblockscheck: portdisk.img | build
	$(CC) -std=c11 -O2 -g -Wall -Wextra -Werror -fsanitize=undefined -fno-sanitize-recover=undefined -Isrc $(M140_IMAGE_SRCS) $(M139_DIR_SRCS) $(M138_MFD_SRCS) $(M137_INDEX_SRCS) $(M126_DQ_READ_SRCS) $(M125_DQ_SRCS) $(M118_DB_SRCS) src/vms_qio_irp_prepare.c tests/test_dq_dir_multiblock.c -o build/dqdirblockscheck
	./build/dqdirblockscheck


# M169-M176: RETRYERR, FUNCXT and IOCIOPOST bounded retry + IOSB, private only.
.PHONY: dqretrycheck
dqretrycheck: $(M168_IOC_OBJS) | build
	$(CC) -std=c11 -O2 -g -Wall -Wextra -Werror -fsanitize=undefined -fno-sanitize-recover=undefined -Isrc -Itests $(M168_IOC_SRCS) tests/test_retry_*_[0123].c tests/test_retry_main.c -o build/dqretrycheck
	./build/dqretrycheck

qemu-sysboot-m199: vms37lab.ima portdisk.img
	python3 tools/qemu_sysboot_m199.py

qemu-sysboot-m201: vms37lab.ima portdisk.img
	python3 tools/qemu_sysboot_m201.py

qemu-sysboot-m202: vms37lab.ima portdisk.img
	python3 tools/qemu_sysboot_m202.py

qemu-sysboot-m203: vms37lab.ima portdisk.img
	python3 tools/qemu_sysboot_m203.py

qemu-sysboot-m204: vms37lab.ima portdisk.img
	python3 tools/qemu_sysboot_m204.py

qemu-sysboot-m205: vms37lab.ima portdisk.img
	python3 tools/qemu_sysboot_m205.py

qemu-sysboot-m206: vms37lab.ima portdisk.img
	python3 tools/qemu_sysboot_m206.py

qemu-sysboot-m207: vms37lab.ima portdisk.img
	python3 tools/qemu_sysboot_m207.py

qemu-sysboot-m209: vms37lab.ima portdisk.img
	python3 tools/qemu_sysboot_m209.py

qemu-sysboot-m210: vms37lab.ima portdisk.img
	python3 tools/qemu_sysboot_m210.py

qemu-sysboot-m211: vms37lab.ima portdisk.img
	python3 tools/qemu_sysboot_m211.py

qemu-sysboot-m212: vms37lab.ima portdisk.img
	python3 tools/qemu_sysboot_m212.py

qemu-sysboot-m213: vms37lab.ima portdisk.img
	python3 tools/qemu_sysboot_m213.py

qemu-sysboot-m214: vms37lab.ima portdisk.img
	python3 tools/qemu_sysboot_m214.py

qemu-sysboot-m216: vms37lab.ima portdisk.img
	python3 tools/qemu_sysboot_m216.py

qemu-sysboot-m217: vms37lab.ima portdisk.img
	python3 tools/qemu_sysboot_m217.py

