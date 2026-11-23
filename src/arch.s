/* Native IA-32 privilege transitions and interrupt entry.
 * Original x86 implementation; no DEC source code.
 */
.section .data,"aw"
.align 8
.global port_gdt
port_gdt:
    .quad 0x0000000000000000  /* 0x00 null */
    .quad 0x00cf9a000000ffff  /* 0x08 CPL0 code */
    .quad 0x00cf92000000ffff  /* 0x10 CPL0 data */
    .quad 0x00cfba000000ffff  /* 0x18 CPL1 code */
    .quad 0x00cfb2000000ffff  /* 0x20 CPL1 data */
    .quad 0x00cfda000000ffff  /* 0x28 CPL2 code */
    .quad 0x00cfd2000000ffff  /* 0x30 CPL2 data */
    .quad 0x00cffa000000ffff  /* 0x38 CPL3 code */
    .quad 0x00cff2000000ffff  /* 0x40 CPL3 data */
    .quad 0x0000000000000000  /* 0x48 dynamically installed 32-bit TSS */
port_gdt_end:
gdt_ptr:
    .word port_gdt_end - port_gdt - 1
    .long port_gdt

.section .text,"ax"
.code32
.global arch_install_gdt
arch_install_gdt:
    lgdt gdt_ptr
    /* Current descriptors stay valid: selectors 0x08 / 0x10 are unchanged.
     * Reload SS/DS immediately; CS is refreshed at a later far jump.
     */
    movw $0x10,%ax
    movw %ax,%ds
    movw %ax,%es
    movw %ax,%ss
    ret

.global arch_load_tr
arch_load_tr:
    movw $0x48,%ax
    ltr %ax
    ret

.extern arch_trap
.global arch_isr_table
.macro vector_no_error n
    .global arch_isr_\n
arch_isr_\n:
    pushl $0                  /* synthetic error code */
    pushl $\n                /* vector number */
    jmp arch_isr_common
.endm
.macro vector_error n
    .global arch_isr_\n
arch_isr_\n:
    pushl $\n                /* CPU pushed an error code already */
    jmp arch_isr_common
.endm
/* CPU exception vectors that automatically push an error code: 8,
 * 10..14, 17. Vector 30 can on newer CPUs, but this Pentium III target
 * has no such exception. */
vector_no_error 0
vector_no_error 1
vector_no_error 2
vector_no_error 3
vector_no_error 4
vector_no_error 5
vector_no_error 6
vector_no_error 7
vector_error 8
vector_no_error 9
vector_error 10
vector_error 11
vector_error 12
vector_error 13
vector_error 14
vector_no_error 15
vector_no_error 16
vector_error 17
vector_no_error 18
vector_no_error 19
vector_no_error 20
vector_no_error 21
vector_no_error 22
vector_no_error 23
vector_no_error 24
vector_no_error 25
vector_no_error 26
vector_no_error 27
vector_no_error 28
vector_no_error 29
vector_no_error 30
vector_no_error 31
vector_no_error 32     /* PIC IRQ0: PIT */
vector_no_error 33     /* PIC IRQ1: keyboard, masked */
vector_no_error 46     /* PIC IRQ14: primary ATA channel */
vector_no_error 128    /* user->kernel syscall gate */
vector_no_error 144    /* VMS 3.7 software IPL4, native CPL0 only */
arch_isr_common:
    cld
    pushal
    /* Interrupts do not swap DS/ES: save user selectors before loading
     * kernel selectors. The C trap-frame pointer still targets PUSHAL. */
    pushl %ds
    pushl %es
    pushl %fs
    pushl %gs
    movw $0x10,%ax
    movw %ax,%ds
    movw %ax,%es
    movw %ax,%fs
    movw %ax,%gs
    leal 16(%esp),%eax
    pushl %eax
    call arch_trap
    addl $4,%esp
    testl %eax,%eax
    jnz arch_ring3_resume
    popl %gs
    popl %fs
    popl %es
    popl %ds
    popal
    addl $8,%esp          /* discard vector and (real/synthetic) error */
    iret

/* Asynchronous return from a ring-3 EXIT system call, without inventing a
 * fake CPL0 IRET frame. arch_enter_ring3 saved the kernel C-call frame. */
.global arch_enter_ring3
arch_enter_ring3:
    pushl %ebp
    pushl %ebx
    pushl %esi
    pushl %edi
    movl %esp,arch_user_return_esp
    pushfl
    popl arch_user_return_eflags
    movl 20(%esp),%eax    /* entry address */
    movl 24(%esp),%edx    /* top of private user stack */
    movl 28(%esp),%esi    /* AST callback parameter, if applicable */
    pushl $0x43           /* CPL3 data selector */
    pushl %edx
    pushfl
    orl $0x200,(%esp)    /* allow PIT interrupts from user mode */
    pushl $0x3b           /* CPL3 code selector */
    pushl %eax
    /* DS/ES/FS/GS must all be user selectors at CPL3. */
    movw $0x43,%ax
    movw %ax,%ds
    movw %ax,%es
    movw %ax,%fs
    movw %ax,%gs
    iret
arch_ring3_resume:
    /* Called only when the trap dispatcher authorizes user EXIT. */
    movl arch_user_return_esp,%esp
    movw $0x10,%ax
    movw %ax,%ds
    movw %ax,%es
    movw %ax,%fs
    movw %ax,%gs
    popl %edi
    popl %esi
    popl %ebx
    popl %ebp
    pushl arch_user_return_eflags
    popfl                       /* restore IF after DPL3 gate EXIT */
    ret
.section .bss,"aw",@nobits
.align 4
arch_user_return_esp:
    .long 0
arch_user_return_eflags:
    .long 0
.section .text,"ax"


/* Ring-3 probes use their own page (U/S bit set in page tables). Nothing
 * here calls kernel C functions or writes supervisor-owned memory. */
.section .user_text,"ax"
.balign 4096
.global arch_user_code_begin
arch_user_code_begin:
.global arch_user_basic
arch_user_basic:
    movl $1,%eax          /* SYSCALL_REPORT_CPL */
    xorl %ebx,%ebx
    movw %cs,%bx
    andl $3,%ebx
    int $0x80
    xorl %eax,%eax        /* SYSCALL_EXIT */
    int $0x80
    ud2                    /* ERROR: the EXIT must never return here */
/* Ring3 system service buffer validation: kernel must reject supervisor
 * addresses, permit user stack write, and reject text write. */
.global arch_user_ptrprobe
arch_user_ptrprobe:
    xorl %esi,%esi
    leal -16(%esp),%ebx
    movl $4,%ecx
    movl $1,%edx
    movl $16,%eax
    int $0x80
    testl %eax,%eax
    jne 1f
    orl $1,%esi
1:  movl $0x10000,%ebx
    movl $4,%ecx
    movl $1,%edx
    movl $16,%eax
    int $0x80
    cmpl $-1,%eax
    jne 2f
    orl $2,%esi
2:  movl $arch_user_basic,%ebx
    movl $4,%ecx
    movl $1,%edx
    movl $16,%eax
    int $0x80
    cmpl $-1,%eax
    jne 3f
    orl $4,%esi
3:  movl $arch_user_basic,%ebx
    movl $4,%ecx
    xorl %edx,%edx
    movl $16,%eax
    int $0x80
    testl %eax,%eax
    jne 4f
    orl $8,%esi
4:  movl %esi,%ebx
    movl $17,%eax
    int $0x80
    xorl %eax,%eax
    int $0x80
    ud2
/* M28 ring3 -> kernel -> ring3 channel ownership ABI smoke test. */
/* M33: read user stack and read-only code through checked CR3/PTE aliases.
 * Reject supervisor and null addresses without touching kernel memory. */
.global arch_user_copyin_probe
arch_user_copyin_probe:
    movl $0xA37B33CC,-4(%esp)
    xorl %esi,%esi
    leal -4(%esp),%ebx
    movl $27,%eax
    int $0x80
    testl %eax,%eax
    jne 70f
    orl $1,%esi
70: movl $arch_user_basic,%ebx
    movl $27,%eax
    int $0x80
    testl %eax,%eax
    jne 71f
    orl $2,%esi
71: movl $0x10000,%ebx
    movl $27,%eax
    int $0x80
    cmpl $-1,%eax
    jne 72f
    orl $4,%esi
72: xorl %ebx,%ebx
    movl $27,%eax
    int $0x80
    cmpl $-1,%eax
    jne 73f
    orl $8,%esi
73: movl %esi,%ebx
    movl $28,%eax
    int $0x80
    xorl %eax,%eax
    int $0x80
    ud2
.global arch_user_channel_probe
arch_user_channel_probe:
    xorl %esi,%esi
    leal -16(%esp),%ebx
    movl $18,%eax
    int $0x80            /* ASSIGN uses writable stack output */
    testl %eax,%eax
    jne 10f
    orl $1,%esi
10: movl -16(%esp),%ebx
    movl $19,%eax
    int $0x80            /* DEASSIGN released channel */
    testl %eax,%eax
    jne 11f
    orl $2,%esi
11: movl -16(%esp),%ebx
    movl $19,%eax
    int $0x80            /* stale handle rejected */
    cmpl $-1,%eax
    jne 12f
    orl $4,%esi
12: movl $0x10000,%ebx
    movl $18,%eax
    int $0x80            /* supervisor output ptr rejected */
    cmpl $-1,%eax
    jne 13f
    orl $8,%esi
13: movl %esi,%ebx
    movl $21,%eax
    int $0x80
    xorl %eax,%eax
    int $0x80
    ud2
/* M29: ring3 owned read-only block QIO submit and wait. User stack
 * supplies buffer, IOSB and channel result; no direct kernel memory. */
.global arch_user_qio_probe
arch_user_qio_probe:
    subl $544,%esp          /* 512 data + 8 IOSB + 4 channel */
    xorl %edi,%edi
    leal 528(%esp),%ebx     /* channel handle output */
    movl $18,%eax
    int $0x80
    testl %eax,%eax
    jne 40f
    orl $1,%edi
40: movl 528(%esp),%ebx
    movl %esp,%ecx         /* buffer writable on user stack */
    leal 512(%esp),%edx    /* IOSB writable on user stack */
    movl $1,%esi           /* LBA1 in test IDE disk */
    movl $22,%eax
    int $0x80
    testl %eax,%eax
    jne 41f
    orl $2,%edi
41: movl 528(%esp),%ebx
    movl $23,%eax
    int $0x80             /* complete deferred block read */
    testl %eax,%eax
    jne 42f
    cmpl $0x33534d56,(%esp)   /* VMS3 magic */
    jne 42f
    cmpl $0x02000001,512(%esp) /* success 1, transferred 512 */
    jne 42f
    orl $4,%edi
42: movl 528(%esp),%ebx
    movl $19,%eax
    int $0x80             /* handle released; deassign */
    testl %eax,%eax
    jne 43f
    orl $8,%edi
43: movl %edi,%ebx
    movl $24,%eax
    int $0x80
    addl $544,%esp
    xorl %eax,%eax
    int $0x80
    ud2
/* M35: pointer-based descriptor is copied from CPL3 and checked before
 * issuing owned QIO. A supervisor descriptor must fail without a read. */
.global arch_user_qio_desc_probe
arch_user_qio_desc_probe:
    subl $560,%esp
    xorl %edi,%edi
    leal 544(%esp),%ebx      /* output handle */
    movl $18,%eax
    int $0x80
    testl %eax,%eax
    jne 80f
    orl $1,%edi
80: movl 544(%esp),%ebx
    movl $0x10000,%ecx       /* forbidden supervisor descriptor */
    movl $29,%eax
    int $0x80
    cmpl $-1,%eax
    jne 81f
    orl $2,%edi
81: movl $1,532(%esp)     /* LBA */
    movl %esp,%eax
    movl %eax,536(%esp)    /* data buffer virtual pointer */
    leal 512(%esp),%eax
    movl %eax,540(%esp)    /* IOSB virtual pointer */
    movl 544(%esp),%ebx
    leal 532(%esp),%ecx
    movl $29,%eax
    int $0x80
    testl %eax,%eax
    jne 82f
    orl $4,%edi
82: movl 544(%esp),%ebx
    movl $23,%eax
    int $0x80
    testl %eax,%eax
    jne 83f
    cmpl $0x33534d56,(%esp)
    jne 83f
    cmpl $0x02000001,512(%esp)
    jne 83f
    orl $8,%edi
83: movl 544(%esp),%ebx
    movl $19,%eax
    int $0x80
    testl %eax,%eax
    jne 84f
    orl $16,%edi
84: movl %edi,%ebx
    movl $30,%eax
    int $0x80
    addl $560,%esp
    xorl %eax,%eax
    int $0x80
    ud2
/* M31: reject invalid CPL3 buffer; revoke queued owned QIO and deassign. */
.global arch_user_qio_cancel_probe
arch_user_qio_cancel_probe:
    subl $544,%esp
    xorl %edi,%edi
    leal 528(%esp),%ebx
    movl $18,%eax
    int $0x80
    testl %eax,%eax
    jne 50f
    orl $1,%edi
50: movl 528(%esp),%ebx
    movl $0x10000,%ecx     /* supervisor output must be rejected */
    leal 512(%esp),%edx
    movl $1,%esi
    movl $22,%eax
    int $0x80
    cmpl $-1,%eax
    jne 51f
    orl $2,%edi
51: movl 528(%esp),%ebx
    movl %esp,%ecx
    leal 512(%esp),%edx
    movl $1,%esi
    movl $22,%eax
    int $0x80
    testl %eax,%eax
    jne 52f
    orl $4,%edi
52: movl 528(%esp),%ebx
    movl $19,%eax         /* outstanding request must pin channel */
    int $0x80
    cmpl $-4,%eax
    jne 53f
    orl $8,%edi
53: movl 528(%esp),%ebx
    movl $25,%eax         /* cancel queued I/O + reclaim request */
    int $0x80
    testl %eax,%eax
    jne 54f
    orl $16,%edi
54: movl 528(%esp),%ebx
    movl $19,%eax
    int $0x80             /* must now deassign */
    testl %eax,%eax
    jne 55f
    orl $32,%edi
55: movl %edi,%ebx
    movl $26,%eax
    int $0x80
    addl $544,%esp
    xorl %eax,%eax
    int $0x80
    ud2
/* M32: deliberately exit CPL3 with outstanding owned QIO; kernel reaps. */
.global arch_user_qio_teardown_probe
arch_user_qio_teardown_probe:
    subl $544,%esp
    leal 528(%esp),%ebx
    movl $18,%eax
    int $0x80
    testl %eax,%eax
    jne 60f
    movl 528(%esp),%ebx
    movl %esp,%ecx
    leal 512(%esp),%edx
    movl $1,%esi
    movl $22,%eax
    int $0x80
60: addl $544,%esp
    xorl %eax,%eax
    int $0x80            /* task ends, outstanding I/O must be reaped */
    ud2
.global arch_user_fault
arch_user_fault:
    movl $1,%eax          /* SYSCALL_REPORT_CPL */
    xorl %ebx,%ebx
    movw %cs,%bx
    andl $3,%ebx
    int $0x80
.global arch_user_fault_site
arch_user_fault_site:
    movl $0xabcdef01,0x10000  /* write supervisor page: must cause #PF */
.global arch_user_fault_after
arch_user_fault_after:
    xorl %eax,%eax
    int $0x80             /* SYSCALL_EXIT */
    ud2
/* A callback trampoline entered at true CPL3 from a queued AST token.
 * The signal goes through the syscall gate; no direct kernel data access. */
.global arch_user_ast
arch_user_ast:
    movl %esi,%ebx       /* AST parameter passed in user register */
    movl $2,%eax         /* SYSCALL_AST_REPORT */
    int $0x80
    xorl %eax,%eax
    int $0x80            /* back to the waiting kernel dispatcher */
    ud2
/* Two independent CPL3 contexts. EAX=4 reports task marker in EBX;
 * EAX=3 yields into the software scheduler; EAX=0 completes the task. */
.global arch_user_task_a
arch_user_task_a:
    movl $3,%ecx
1:  movl $1,%ebx
    movl $4,%eax
    int $0x80
    movl $3,%eax
    int $0x80
    decl %ecx
    jnz 1b
    xorl %eax,%eax
    int $0x80
    ud2
.global arch_user_task_b
arch_user_task_b:
    movl $3,%ecx
2:  movl $2,%ebx
    movl $4,%eax
    int $0x80
    movl $3,%eax
    int $0x80
    decl %ecx
    jnz 2b
    xorl %eax,%eax
    int $0x80
    ud2
/* Timer-preempted tasks do not issue the cooperative yield syscall.
 * All task clock loads use the user-RO alias of the shared tick page. */
.global arch_user_preempt_a
arch_user_preempt_a:
    movl $3,%ecx
    movl $arch_user_clock_page,%edi
10: movl (%edi),%edx
    addl $2,%edx
11: cmpl %edx,(%edi)
    jb 11b
    movl $1,%ebx
    movl $4,%eax
    int $0x80
    decl %ecx
    jnz 10b
    xorl %eax,%eax
    int $0x80
    ud2
.global arch_user_preempt_b
arch_user_preempt_b:
    movl $3,%ecx
    movl $arch_user_clock_page,%edi
20: movl (%edi),%edx
    addl $2,%edx
21: cmpl %edx,(%edi)
    jb 21b
    movl $2,%ebx
    movl $4,%eax
    int $0x80
    decl %ecx
    jnz 20b
    xorl %eax,%eax
    int $0x80
    ud2
/* Return-frame rewriting supplies ESI with the AST parameter. The
 * COMPLETE syscall restores saved GPRs, PC, flags and user SP exactly. */
.global arch_user_auto_ast
arch_user_auto_ast:
    movl %esi,%ebx
    movl $2,%eax
    int $0x80
    movl $6,%eax
    int $0x80
    ud2
.global arch_user_isolation
arch_user_isolation:
.global arch_user_iso_fault_site
arch_user_iso_fault_site:
    movl $0xdeadbeef,task_b_stack
.global arch_user_iso_fault_after
arch_user_iso_fault_after:
    xorl %eax,%eax
    int $0x80
    ud2
/* M6: independent physical pages mapped to the SAME virtual address in
 * two separate CR3 spaces. Both tasks yield before reading their markers. */
.global arch_user_mem_a
arch_user_mem_a:
    movl $0xA37A0001,0x00300000
    movl $3,%eax
    int $0x80
    movl 0x00300000,%ebx
    movl $7,%eax
    int $0x80
    xorl %eax,%eax
    int $0x80
    ud2
.global arch_user_mem_b
arch_user_mem_b:
    movl $0xB37B0002,0x00300000
    movl $3,%eax
    int $0x80
    movl 0x00300000,%ebx
    movl $7,%eax
    int $0x80
    xorl %eax,%eax
    int $0x80
    ud2
/* M8: two tasks touch two initially non-present user pages.
 * Reads must cause #PF and return zero after physical allocation;
 * EIP is retried, then each task writes private signatures. */
.global arch_user_demand_a
arch_user_demand_a:
    movl 0x00310000,%ebx
    orl 0x00311000,%ebx
    movl $8,%eax
    int $0x80
    movl $0xA37A0001,0x00310000
    movl $0xA37A1001,0x00311000
    movl $3,%eax
    int $0x80
    movl 0x00310000,%ebx
    movl $7,%eax
    int $0x80
    movl 0x00311000,%ebx
    movl $9,%eax
    int $0x80
    xorl %eax,%eax
    int $0x80
    ud2
.global arch_user_demand_b
arch_user_demand_b:
    movl 0x00310000,%ebx
    orl 0x00311000,%ebx
    movl $8,%eax
    int $0x80
    movl $0xB37B0002,0x00310000
    movl $0xB37B1002,0x00311000
    movl $3,%eax
    int $0x80
    movl 0x00310000,%ebx
    movl $7,%eax
    int $0x80
    movl 0x00311000,%ebx
    movl $9,%eax
    int $0x80
    xorl %eax,%eax
    int $0x80
    ud2
/* M9: A blocks until two shared event bits are set by B. B's OR-wait
 * succeeds immediately, then B yields to let woken A finish. */
.global arch_user_event_a
arch_user_event_a:
    movl $3,%ebx
    movl $13,%eax       /* WAIT_AND, should block */
    int $0x80
    movl $0xEFA37001,%ebx
    movl $15,%eax
    int $0x80
    xorl %eax,%eax
    int $0x80
    ud2
.global arch_user_event_b
arch_user_event_b:
    movl $1,%ebx
    movl $10,%eax       /* set bit 0; A must remain blocked */
    int $0x80
    movl $2,%ebx
    movl $10,%eax       /* set bit 1; A becomes READY */
    int $0x80
    movl $3,%ebx
    movl $12,%eax       /* WAIT_OR immediate success */
    int $0x80
    movl $0xEFB37002,%ebx
    movl $15,%eax
    int $0x80
    movl $3,%eax        /* yield to A */
    int $0x80
    xorl %eax,%eax
    int $0x80
    ud2
/* Two independent ring-3 tasks exercise lock contention and resource sleep. */
.global arch_user_mutex_a
arch_user_mutex_a:
    xorl %edi,%edi
    movl $1,%ebx            /* writer lock */
    movl $35,%eax
    int $0x80
    testl %eax,%eax
    jne 520f
    orl $1,%edi
520: movl $3,%eax            /* allow B to try while A holds the lock */
    int $0x80
    movl $36,%eax
    int $0x80
    testl %eax,%eax
    jne 521f
    orl $2,%edi
521: movl $3,%eax            /* B retries and holds read lock */
    int $0x80
    movl $3,%ebx            /* resource wait until B announces availability */
    movl $38,%eax
    int $0x80
    testl %eax,%eax
    jne 522f
    orl $4,%edi
522: movl %edi,%ebx
    movl $37,%eax
    int $0x80
    xorl %eax,%eax
    int $0x80
    ud2
.global arch_user_mutex_b
arch_user_mutex_b:
    xorl %edi,%edi
    movl $2,%ebx            /* write NOWAIT must fail */
    movl $35,%eax
    int $0x80
    cmpl $2,%eax
    jne 523f
    orl $1,%edi
523: xorl %ebx,%ebx         /* read lock waits behind A's writer */
    movl $35,%eax
    int $0x80
    cmpl $1,%eax            /* returns here only after scheduler wakes B */
    jne 524f
    orl $2,%edi
524: xorl %ebx,%ebx         /* retry the lock after wakeup */
    movl $35,%eax
    int $0x80
    testl %eax,%eax
    jne 525f
    orl $4,%edi
525: movl $36,%eax
    int $0x80
    testl %eax,%eax
    jne 526f
    orl $8,%edi
526: movl $3,%eax            /* A blocks in RWAIT */
    int $0x80
    movl $3,%ebx
    movl $39,%eax
    int $0x80
    cmpl $1,%eax
    jne 527f
    orl $16,%edi
527: movl %edi,%ebx
    movl $37,%eax
    int $0x80
    xorl %eax,%eax
    int $0x80
    ud2
.global arch_user_code_end
arch_user_code_end:
.section .text,"ax"

/* Hardware IDT smoke tests: the only recoverable kernel fault sites. */
.global arch_probe_int3
.global arch_probe_int3_after
arch_probe_int3:
    int3
arch_probe_int3_after:
    ret
.global arch_probe_ud2
arch_probe_ud2:
    ud2
    ret

.section .rodata,"a"
.align 4
arch_isr_table:
.irp v,0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,17,18,19,20,21,22,23,24,25,26,27,28,29,30,31,32,33,128,144,46
    .long arch_isr_\v
.endr
.section .note.GNU-stack,"",@progbits
