/* Native PC BOO$BLOCK loads a recorded count of blocks into memory.
 * It enters IA-32 protected mode and transfers to the native kernel.
 * No DOS, VAX instruction emulator, or guest kernel is involved.
 */
.section .text
.code16
.global _start
_start:
    cli
    xorw %ax, %ax
    movw %ax, %ds
    movw %ax, %ss
    movw $0x7c00, %sp
    movb %dl, boot_drive
    sti
    /* VMB.MAR probes memory before passing its page map to SYSBOOT.
     * The native PC obtains physical ranges from BIOS INT 15h/E820;
     * a missing BIOS map is an error, not an excuse to guess memory. */
    xorw %ax, %ax
    movw %ax, %es
    movw $0, 0x4ff0
    movw $0x5000, %di
    xorl %ebx, %ebx
vmb_e820_next:
    movl $0xe820, %eax
    movl $0x534d4150, %edx
    movl $20, %ecx
    int $0x15
    jc vmb_e820_invalid
    cmpl $0x534d4150, %eax
    jne vmb_e820_invalid
    cmpl $20, %ecx
    jb vmb_e820_invalid
    incw 0x4ff0
    addw $20, %di
    testl %ebx, %ebx
    jz vmb_e820_done
    cmpw $0x5280, %di
    jb vmb_e820_next
    /* The map may not be truncated and then treated as complete. */
vmb_e820_invalid:
    movw $0xffff, 0x4ff0
vmb_e820_done:

    /* BOOTS/SRC/BOOTBLOCK.MAR: BOO$BLOCK_CODE, READ_BLOCK, NEXT_BLOCK.
     * The disk records the count and first LBN. BIOS disk reads replace
     * the VAX device ROM. Each request transfers one full 512-byte block.
     */
    movw stage2_count, %bp
    testw %bp, %bp
    jz disk_error
    cmpw $260, %bp
    ja disk_error
    movw stage2_lbn, %di
    cmpw $1, %di
    jb disk_error
    movw %di, %ax
    addw %bp, %ax
    jc disk_error
    cmpw $2880, %ax
    ja disk_error
    movw $0x1000, %si
read_block:
    movw %di, %ax
    xorw %dx, %dx
    movw $36, %bx
    divw %bx
    movb %al, %ch
    movw %dx, %ax
    movb $18, %bl
    divb %bl
    movb %al, %dh
    movb %ah, %cl
    incb %cl
    movw %si, %ax
    movw %ax, %es
    xorw %bx, %bx
    movb $0x02, %ah
    movb $1, %al
    movb boot_drive, %dl
    call bios_read_retry
next_block:
    addw $32, %si
    incw %di
    decw %bp
    jnz read_block

    /* Fast A20 gate for the 1..3 MiB physical page allocator. Port 0x92
     * bit 1 enables A20; bit 0 is a fast-reset trap, always clear it. */
    inb $0x92, %al
    orb $0x02, %al
    andb $0xfe, %al
    outb %al, $0x92

    /* BIOS AL is not a portable transfer count: rely on CF only. */

    cli
    lgdt gdt_descriptor
    movl %cr0, %eax
    orl $1, %eax
    movl %eax, %cr0
    ljmpl $0x08, $protected_entry

.code32
protected_entry:
    movw $0x10, %ax
    movw %ax, %ds
    movw %ax, %es
    movw %ax, %fs
    movw %ax, %gs
    movw %ax, %ss
    /* Keep the growing bootstrap stack above the kernel's BSS.  The BSS
     * limit remains below 0x90000; kmain and resident SYS calls can nest
     * several KiB before switching to per-process stacks. */
    movl $0x0009f000, %esp
    ljmp $0x08, $0x00010000

.code16
/* AH=02h, AL=count, ES:BX=destination, CH/CL/DH=CHS, DL=drive.
 * Preserve the complete request for every BIOS attempt/reset and limit
 * errors to three tries. This does not hide a persistent media failure. */
bios_read_retry:
    pushw %si
    movw $3, %si
.retry:
    pushw %ax
    pushw %bx
    pushw %cx
    pushw %dx
    pushw %es
    int $0x13
    jnc .success
    popw %es
    popw %dx
    popw %cx
    popw %bx
    popw %ax
    /* Reset this drive without destroying the saved CHS read request. */
    pushw %ax
    pushw %bx
    pushw %cx
    pushw %dx
    pushw %es
    xorw %ax, %ax
    int $0x13
    popw %es
    popw %dx
    popw %cx
    popw %bx
    popw %ax
    decw %si
    jnz .retry
    popw %si
    jmp disk_error
.success:
    popw %es
    popw %dx
    popw %cx
    popw %bx
    popw %ax
    popw %si
    ret

disk_error:
    movw $0x0003, %ax   /* BIOS 80x25 color text */
    int $0x10
    movw $error_text, %si
print_error:
    lodsb
    testb %al, %al
    jz freeze
    movb $0x0e, %ah
    movw $0x0007, %bx
    int $0x10
    jmp print_error
freeze:
    cli
    hlt
    jmp freeze

.align 8
gdt:
    .quad 0x0000000000000000
    .quad 0x00cf9a000000ffff   /* 0x08: flat 4GiB 32-bit code */
    .quad 0x00cf92000000ffff   /* 0x10: flat 4GiB data */
gdt_end:
gdt_descriptor:
    .word gdt_end - gdt - 1
    .long gdt
boot_drive:
    .byte 0
error_text:
    .asciz "Disk read failed.\r\n"
/* Original BOO$BLOCK stats: block count, starting LBN and load address.
 * The PC sector stores its stats here rather than in the first 12 bytes;
 * BIOS starts executing code at byte zero on the PC. */
stage2_magic:
    .ascii "VMSB"
stage2_count:
    .word 0
stage2_lbn:
    .word 1

.org 510
.word 0xaa55
