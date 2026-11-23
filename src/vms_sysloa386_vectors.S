/* SYS/SRC/SYSLOAVEC.MAR LOAVEC original 23-entry order.
 * Type 2 entries map VAX CPU/bus exception hardware and deliberately fail
 * on this PC. EXE$INIBOOTADP and EXE$INIPROCREG have real IA-32 services.
 * This is a self-relative table loaded from the actual Files-11 file. */
.section .vmsvec,"a",@progbits
.balign 4
.global port_sysloa386_descriptor
port_sysloa386_descriptor:
    .long port_sysloa386_image_end - port_sysloa386_descriptor
    .long 0
    .short port_sysloa386_image_end - port_sysloa386_descriptor
    .byte 98,0
.macro VEC kind, dest
    .byte \kind
1:  .long \dest - 1b
.endm
/* Longword-aligned original interrupt/machine-check entries. */
.rept 7
    VEC 2, port_sysloa386_unavailable
.endr
/* ECC$REENABLE (VAX hardware), then EXE$INIBOOTADP (native ATA). */
    VEC 3, port_sysloa386_unavailable
    VEC 3, port_sysloa386_boot_probe
/* EXE$DUMPCPUREG, REGRESTOR, REGSAVE are still unported. */
.rept 3
    VEC 3, port_sysloa386_unavailable
.endr
/* EXE$INIPROCREG inspects real SIDT/CR0/CR3/TSS and PIT IRQ. */
    VEC 3, port_sysloa386_cpu_probe
/* EXE$TEST_CSR, PURGDATAP, UBA/CI/MBA and shutdown remain unported. */
.rept 9
    VEC 3, port_sysloa386_unavailable
.endr
/* Data pointer EXE$MCHK_ERRCNT (resident, zero-initialized counters). */
    VEC 1, port_sysloa386_mcheck_errors
    .long -1
.section .note.GNU-stack,"",@progbits
